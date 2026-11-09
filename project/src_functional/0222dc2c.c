/* actor_anim_frame_update [0x0222dc2c]: Advances pose interpolation, root motion and animation events before resolving movement. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222dc2c  actor_anim_frame_update
 *
 * Per-frame animation driver for one actor: advance the animation frame, pick
 * the source pose records, (re)prime the three 108-byte working pose buffers,
 * interpolate the current pose toward the next one, fold the result into the
 * actor's world position, apply the movement/transform selected by the pose's
 * motion mode, run the post-update hook, and finally fire any one-shot
 * animation events (spawn / child / combo) attached to the active frame.
 *   arg0 = actor/task handle (passed on to the event spawns)
 *   arg1 = entity.   returns func_022170ea(entity).
 *
 * Source records (table @0x0607D124, stride 28):
 *   recA = 0x0607D124 + (u16)entity->unk5A * 28   (current anim)
 *   recB = 0x0607D124 + (u16)entity->unk5C * 28   (next/queued anim)
 * Working pose buffers (stride 108), all offset by  off = (s16)((s8)unk34*108):
 *   buf_cur  @0x06096328   buf_prev @0x0609661C   buf_next @0x06096910
 *   rec_out  @0x06096388   (transformed root position, 3 longs)
 * var_r12 is the interpolation shift count: *0x06096C90 (global base) + a
 * per-record field; unk40 = 1<<var_r12 is the step/weight (decremented each frame).
 *
 * All 34 computed branches resolved via braf_resolve.py; the lone PC-relative
 * bsr (0x22def8 -> 0x0222e3b0) is the draw-emitting sibling of interp_shorts_48.
 * Call cluster:
 *   0x0224a280 block copy (count,dst,src)   0x0224a360 ashl_var(val,cnt)=val<<cnt
 *   0x0224a420 ashr_var(val,cnt)=val>>cnt    0x02213390 interp_shorts_48(rec,sh,w)
 *   0x0222e3b0 interp+draw sibling           0x02213fb4 load_matrix_default()
 *   0x02214a06 mtx_rotate_zyx_at24(entity)   0x02213538 vec3_matmul_out(x,y,z)
 *   0x022170ea actor post-update (returns)   0x0220ff28 spawn_from_template
 *   0x0220fece spawn child                   0x02215f6c combo sub-table consumer
 *
 * fidelity: HIGH on control flow and the resolved call sequence (branch tree,
 * the 5-way motion switch, the 3-component interpolation incl. the ashr shift
 * m2c dropped, and the event-dispatch tail all transcribed from the SH-2 asm).
 * MED on a few helper arg *semantics* (02213538 name/ABI is a cluster note, not
 * re-derived here).  m2c corroborates the whole skeleton; the stray `unk40 = 1`
 * and `return var_r0_5` m2c emitted are delay-slot/unresolved-bsrf artifacts and
 * are corrected here.
 */
extern void shc_copy_word_block(s32 count, void *dst, void *src);  /* block copy */
extern s32  shc_shift_left(s32 value, s32 count);             /* ashl_var */
extern s32  shc_shift_right_signed(s32 value, s32 count);             /* ashr_var */
extern void interp_shorts_48(s32 rec, s32 shift, s32 weight);   /* interp_shorts_48 */
extern void pose_blend_rotation_matrices(s32 rec, s32 shift, s32 weight);   /* interp + draw */
extern void load_matrix_default(void);                             /* load_matrix_default */
extern void mtx_rotate_zyx_at24(void *entity);                     /* mtx_rotate_zyx_at24 */
extern void matrix_transform_to_scratch(s32 x, s32 y, s32 z);              /* vec3_matmul_out */
extern s32  actor_resolve_movement(void *entity);                     /* actor post-update */
extern void *task_spawn_list_work7c(s32 arg0, s32 param);             /* spawn_from_template */
extern void task_spawn_current_work6c(s32 arg0, s32 param);              /* spawn child */
extern void sound_queue_push(s32 packed);                       /* combo event */

#define GB    0x0607D124u               /* anim-record table base (stride 28) */
#define MTX   M32((void *)DH_MATRIX_STACK_CURSOR_ADDR_U, 0)   /* matrix-stack top pointer */
#define SHBASE M32((void *)0x06096C90u, 0)  /* interpolation shift base */

s32 actor_anim_frame_update(s32 arg0, void *entity) {
    u8  *recA = (u8 *)(uintptr_t)(GB + (u16)ACTOR_U16(entity, animation_current) * 28);
    u8  *recB = (u8 *)(uintptr_t)(GB + (u16)ACTOR_U16(entity, animation_requested) * 28);
    s32  u34  = (s8)ACTOR_WORK(entity)->slot_id;
    s32  off  = (s16)(u34 * 108);
    void *buf_cur  = (void *)(uintptr_t)(0x06096328u + off);
    void *buf_prev = (void *)(uintptr_t)(0x0609661Cu + off);
    void *buf_next = (void *)(uintptr_t)(0x06096910u + off);
    void *rec_out  = (void *)(uintptr_t)(0x06096388u + off);
    s32  var_r12;
    s16  sp8 = 0;
    s32  save_pos[3];   /* current root pos latched before interpolation */
    s32  interp[3];     /* interpolated root pos */
    s32  ret;

    ACTOR_WORK(entity)->animation_events = 0;
    var_r12 = SHBASE + (u16)MU16(recA, 0x16);

    if (ACTOR_WORK(entity)->animation_control & DH_ANIMATION_CONTROL_RESTART) {
        /* start a fresh animation */
        sp8 = M16(recA, 0x0A);
        ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_BLENDING;
        shc_copy_word_block(108, buf_prev, buf_cur);
        shc_copy_word_block(108, buf_next, MP(recB, 0x18));
        if ((u16)ACTOR_U16(entity, animation_requested) > 0x22)
            var_r12 = SHBASE + (u16)MU16(recB, 0x10);
        else
            var_r12 = SHBASE + (u16)MU16(recA, 0x12);
        ACTOR_WORK(entity)->interpolation_ticks = (u8)shc_shift_left(1, var_r12);
        ACTOR_WORK(entity)->animation_frame = 0;
    } else if (ACTOR_WORK(entity)->interpolation_ticks != 0) {
        /* mid-interpolation: nothing to advance */
    } else {
        /* advance to the next frame */
        ACTOR_WORK(entity)->animation_frame = (s16)(ACTOR_U16(entity, animation_frame) + 1);
        if (ACTOR_WORK(entity)->animation_control == DH_ANIMATION_CONTROL_ADVANCE_LOOP) {
            ACTOR_WORK(entity)->animation_control = 0;
            ACTOR_WORK(entity)->animation_frame = 1;
            var_r12 = SHBASE + (u16)MU16(recA, 0x16);
            ACTOR_WORK(entity)->interpolation_ticks = (u8)shc_shift_left(1, var_r12);
            shc_copy_word_block(108, buf_prev, buf_next);
            if (ACTOR_U16(entity, animation_frame) == MU16(recA, 0x14)) {
                sp8 = M16(recA, 0x0A);
                ACTOR_WORK(entity)->animation_frame = 0;
                shc_copy_word_block(108, buf_next, MP(recB, 0x18));
            } else {
                shc_copy_word_block(108, buf_next, (u8 *)MP(recA, 0x18) + 108);
            }
        } else if (ACTOR_U16(entity, animation_frame) == MU16(recA, 0x14)) {
            if ((u16)ACTOR_U16(entity, animation_requested) > 0x22)
                var_r12 = SHBASE + (u16)MU16(recB, 0x10);
            else
                var_r12 = SHBASE + (u16)MU16(recA, 0x12);
            sp8 = M16(recA, 0x0A);
            ACTOR_WORK(entity)->animation_frame = 0;
            ACTOR_WORK(entity)->interpolation_ticks = (u8)shc_shift_left(1, var_r12);
            shc_copy_word_block(108, buf_prev, buf_next);
            shc_copy_word_block(108, buf_next, MP(recB, 0x18));
        } else if (ACTOR_U16(entity, animation_frame) == 1) {
            var_r12 = SHBASE + (u16)MU16(recA, 0x16);
            ACTOR_WORK(entity)->interpolation_ticks = (u8)shc_shift_left(1, var_r12);
            shc_copy_word_block(108, buf_prev, buf_next);
            shc_copy_word_block(108, buf_next, (u8 *)MP(recA, 0x18) + 108);
        } else {
            ACTOR_WORK(entity)->interpolation_ticks = (u8)shc_shift_left(1, var_r12);
            shc_copy_word_block(108, buf_prev, buf_next);
            shc_copy_word_block(108, buf_next,
                          (u8 *)MP(recA, 0x18) + (u16)ACTOR_U16(entity, animation_frame) * 108);
        }
    }

    ACTOR_WORK(entity)->interpolation_ticks = (s8)(ACTOR_WORK(entity)->interpolation_ticks - 1);

    if (ACTOR_WORK(entity)->animation_control & 6) {
        sp8 = M16(recA, 0x0A);
        if ((u16)ACTOR_U16(entity, animation_requested) > 0x22)
            var_r12 = SHBASE + (u16)MU16(recB, 0x10);
        else
            var_r12 = SHBASE + (u16)MU16(recA, 0x12);
    } else if (ACTOR_U16(entity, animation_frame) == 0) {
        sp8 = M16(recA, 0x0A);
        if ((u16)ACTOR_U16(entity, animation_requested) > 0x22)
            var_r12 = SHBASE + (u16)MU16(recB, 0x10);
        else
            var_r12 = SHBASE + (u16)MU16(recA, 0x12);
    }

    /* latch the current transformed root position */
    save_pos[0] = M32(rec_out, 0);
    save_pos[1] = M32(rec_out, 4);
    save_pos[2] = M32(rec_out, 8);

    if (ACTOR_WORK(entity)->interpolation_ticks != 0) {
        if (SHBASE != 0)
            pose_blend_rotation_matrices(u34, var_r12, (u8)ACTOR_WORK(entity)->interpolation_ticks);
        else
            interp_shorts_48(u34, var_r12, (u8)ACTOR_WORK(entity)->interpolation_ticks);
    } else {
        shc_copy_word_block(108, buf_cur, buf_next);
    }

    /* interpolate the 3 root-position components next->prev by weight unk40 */
    {
        s32 i;
        s32 w = (u8)ACTOR_WORK(entity)->interpolation_ticks;
        for (i = 0; i < 3; i++) {
            s32 b = M32(buf_next, 0x60 + i * 4);
            s32 a = M32(buf_prev, 0x60 + i * 4);
            interp[i] = b - shc_shift_right_signed((b - a) * w, var_r12);
        }
    }

    M32((void *)0x06096C58u, u34 * 4) = interp[1];
    M32(rec_out, 0) = interp[0];
    M32(rec_out, 4) = interp[1];
    M32(rec_out, 8) = interp[2];

    switch ((u16)sp8 & 7) {
    case 0:
        if ((ACTOR_WORK(entity)->action_category & 0x01212000) != 0) {
            ACTOR_WORK(entity)->movement_x += M32(entity, 0x78);
            ACTOR_WORK(entity)->movement_y += M32(entity, 0x7C) - ACTOR_WORK(entity)->position_y;
            ACTOR_WORK(entity)->movement_z += M32(entity, 0x80);
        } else {
            MTX += 0x30;
            load_matrix_default();
            mtx_rotate_zyx_at24(entity);
            matrix_transform_to_scratch(interp[0] - save_pos[0], 0, interp[2] - save_pos[2]);
            ACTOR_WORK(entity)->movement_x += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 0);
            ACTOR_WORK(entity)->movement_y += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 4);
            ACTOR_WORK(entity)->movement_z += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 8);
            MTX -= 0x30;
        }
        break;
    case 1:
        if ((u16)ACTOR_U16(entity, animation_requested) > 0x22) {
            if ((ACTOR_WORK(entity)->action_category & 0x01212000) != 0) {
                ACTOR_WORK(entity)->movement_x += M32(entity, 0x78);
                ACTOR_WORK(entity)->movement_y += M32(entity, 0x7C) - ACTOR_WORK(entity)->position_y;
                ACTOR_WORK(entity)->movement_z += M32(entity, 0x80);
            } else {
                MTX += 0x30;
                load_matrix_default();
                mtx_rotate_zyx_at24(entity);
                matrix_transform_to_scratch(interp[0] - save_pos[0], 0, interp[2] - save_pos[2]);
                ACTOR_WORK(entity)->movement_x += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 0);
                ACTOR_WORK(entity)->movement_y += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 4);
                ACTOR_WORK(entity)->movement_z += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 8);
                MTX -= 0x30;
            }
        }
        break;
    case 2:
        if ((u16)ACTOR_U16(entity, animation_requested) <= 0x22) {
            if ((ACTOR_WORK(entity)->action_category & 0x01212000) != 0) {
                ACTOR_WORK(entity)->movement_x += M32(entity, 0x78);
                ACTOR_WORK(entity)->movement_y += M32(entity, 0x7C) - ACTOR_WORK(entity)->position_y;
                ACTOR_WORK(entity)->movement_z += M32(entity, 0x80);
            } else {
                MTX += 0x30;
                load_matrix_default();
                mtx_rotate_zyx_at24(entity);
                matrix_transform_to_scratch(interp[0] - save_pos[0], 0, interp[2] - save_pos[2]);
                ACTOR_WORK(entity)->movement_x += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 0);
                ACTOR_WORK(entity)->movement_y += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 4);
                ACTOR_WORK(entity)->movement_z += M32((void *)DH_TRANSFORMED_VECTOR_ADDR_U, 8);
                MTX -= 0x30;
            }
        }
        break;
    case 4: {
        void *rec = (void *)(uintptr_t)(0x06096C04u + (s8)(u34 * 12));
        ACTOR_WORK(entity)->movement_x += M32(rec, 0);
        ACTOR_WORK(entity)->movement_y += M32(rec, 4);
        ACTOR_WORK(entity)->movement_z += M32(rec, 8);
        break;
    }
    default:  /* cases 3, 5, 6, 7: no movement */
        break;
    }

    {
        void *rec = (void *)(uintptr_t)(0x06096C04u + (s8)(u34 * 12));
        M32(rec, 0) = ACTOR_WORK(entity)->movement_x;
        M32(rec, 4) = ACTOR_WORK(entity)->movement_y;
        M32(rec, 8) = ACTOR_WORK(entity)->movement_z;
    }

    ret = actor_resolve_movement(entity);

    if (ACTOR_WORK(entity)->interpolation_ticks == 0) {
        /* fire the one-shot events attached to the active frame */
        if (ACTOR_WORK(entity)->animation_control == DH_ANIMATION_CONTROL_BLENDING) {
            ACTOR_WORK(entity)->animation_events = 1;
            ACTOR_WORK(entity)->animation_current = ACTOR_WORK(entity)->animation_requested;
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_ADVANCE_LOOP;
            if ((u16)(ACTOR_U16(entity, animation_frame) + 1) == (u16)MU16(recB, 0x14))
                ACTOR_WORK(entity)->animation_events |= (s16)((M16(recA, 0x0A) & 0x1F30) + 2);
        } else {
            if ((u16)(ACTOR_U16(entity, animation_frame) + 1) == (u16)MU16(recA, 0x14))
                ACTOR_WORK(entity)->animation_events = (s16)((M16(recA, 0x0A) & 0x1F30) + 2);
            if (ACTOR_U16(entity, animation_frame) == 0) {
                ACTOR_WORK(entity)->animation_events |= 1;
                ACTOR_WORK(entity)->animation_current = ACTOR_WORK(entity)->animation_requested;
            }
        }

        {
            u8 *recA2 = (u8 *)(uintptr_t)(GB + (u16)ACTOR_U16(entity, animation_current) * 28);

            if (M32(recA2, 4) != 0) {
                if ((u8)M8(recA2, 2) == (u16)ACTOR_U16(entity, animation_frame)) {
                    s32 idx = M8(recA2, 0) & 63;
                    s32 base = M32((void *)0x06091334u,
                                   (s8)M8((void *)0x0609868Eu, 0) * 4);
                    void *child = MP(task_spawn_list_work7c(arg0, M32((void *)(uintptr_t)
                                        (base + idx * 4), 0)), 8);
                    M32(child, 0x68) = M16(recA2, 8) != 0 ? M32(entity, 0xC0) : 0;
                    M8(child, 0x5F)  = (s8)M8(recA2, 1);
                    M16(child, 0x60) = M16(recA2, 8);
                    M16(child, 0x62) = (u8)M8(recA2, 0);
                    M32(child, 0x64) = M32(recA2, 4);
                    M16(child, 0x58) = (u8)M8(recA2, 3);
                }
            } else {
                if (M16(recA2, 8) != 0
                    && (u8)M8(recA2, 2) == (u16)ACTOR_U16(entity, animation_frame))
                    M16(entity, 0x66) = M16(recA2, 8);
            }

            if ((u8)M8(recA2, 0x0C) != 0
                && (u8)M8(recA2, 0x0D) == (u16)ACTOR_U16(entity, animation_frame)
                && (s32)(u8)M8(recA2, 0x0C) > 2) {
                task_spawn_current_work6c(arg0, M32((void *)0x0608C538u,
                                        (u8)M8(recA2, 0x0C) * 4));
            }

            if ((u8)M8(recA2, 0x0E) != 0
                && (u8)M8(recA2, 0x0F) == (u16)ACTOR_U16(entity, animation_frame)
                && M32((void *)DH_SOUND_EFFECTS_ENABLED_ADDR_U, 0) != 0) {
                sound_queue_push(((s32)M16((void *)0x06053F04u,
                                        (u8)M8(recA2, 0x0E) * 2) << 8)
                              | (s32)DH_SOUND_CUE_BASE);
            }
        }
    }

    return ret;
}

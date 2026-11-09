/* actor_tick_scripted_spawner [0x02212350]: Task+0f selects 16-byte script entries; trigger walks entries and spawns projectile tasks. */
#include "dh_work.h"
#include "dh.h"
/* 0x02212350 (actor_update_scripted_spawner) - entity-behaviour state machine driven
 * by a per-task script list g_060906e8[task->unkF] (16-byte entries). The header entry
 * seeds the actor; on trigger it walks the entry list and spawns a batch of child
 * entities, copying the parent transform into each. Installed as a task handler:
 * task->unk8 = entity, task->unk10 = state. Survives via the draw/grid tail func_0222eee4.
 *
 * state 0 (init): from header hdr = script[0] and param block pb = &g_060521f4[hdr->
 *   unk4 * 4]: unkC += pb[1]; zero unk14/18/1C/78/7C/80; unk30 = hdr->unk8; unkA8 =
 *   hdr->unkC + 0x2000; alloc collision slot (func_02228106) -> unk34 (deactivate if
 *   it fails); unk38 = 0x100000; unkBC = 0x7FFFFF00; register slot with pb[0..3];
 *   unk62 = 0; -> state 1, run update.
 * state 1 (update): move/collide via func_022170ea(entity, (hdr->unk0 & 0x10000)?3:1).
 *   When unk48 is set, clear it and - if unk4A < 8 - advance to state 2, derive the
 *   spawn aim vector unk14/18/1C via func_022104f6(unk60, unk62, ...), scale it
 *   (unk14/1C *= unk64<<9; unk18 = (|unk18|>>3) + 0x1400), play the header SFX
 *   (((hdr->unk0 & 0xFFFF) << 8) | 0xA00000FF, gated by g_060E0DDC), then interpret
 *   each list entry e (until e->unk0 < 0):
 *     - e->unk0 & 2 : if a slot is free (func_0223e4a0), spawn func_0223cbb0(e->unk4,
 *       3, 0), scatter unk62 += (rng & 0x1FFF)+0xF000, hand to func_0223a924.
 *     - else e->unk0 & 8 : spawn via func_0220feec/fece/ff28/ff0a selected by e->unk4
 *       (0..3); copy parent pos/angle/aim into child->unk8; child->unk3C = e->unkC;
 *       copy child->unk34 unless e->unk4 == 1.
 *     - else e->unk0 & 4 == 0 : spawn func_0220feec(task, &func_0221279c); copy the
 *       parent state plus child->unk60=e->unkC, unk64=e->unk4, unk30=e->unk8,
 *       unk80=e->unk0, unk36=parent unk4A.
 *   task->unkF is set to the entry index as each is processed.
 * state 2 (cleanup): release_slot_entry(entity); release g_060996D4[unk34-8] (if unk34 > 8);
 *   deactivate (task->unk0 = 0); no draw.
 * other states: no-op.
 *
 * Draw (states 0/1): push matrix, mtx_load_abc, apply matrix to vec @+8, rotate @+0x24,
 * submit model, pop, tail-call func_0222eee4(entity).
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf/braf resolved;
 * mova+add handler pointer resolved to func_0221279c (ptr_resolve). The spawn-switch
 * default reuses the previous child pointer exactly as the SH-2 reuses r10 (obj
 * initialised to 0 so the C is defined; valid scripts only use e->unk4 in 0..3). */

extern s32   actor_allocate_effect_slot(void);
extern void  actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3);
extern void  actor_resolve_movement(void *entity, s32 mode);          /* entity_move_dispatch */
extern void  direction_from_yaw_pitch(s16 a, s16 b, s32 *ox, s32 *oy, s32 *oz); /* aim vector */
extern s32   fixed_mul16(s32 a, s32 b);                    /* fixmul_16_16 */
extern s32   sound_queue_push(u32 sound_id);
extern s32   effect_pool_make_room(void);
extern void *effect_pool_spawn(s32 a0, s32 a1, void *a2);
extern s32   game_random_next(void);                            /* rng_next */
extern void  effect_pool_update_pose(s32 idx, s32 mode, void *src, void *save);
extern void *task_spawn_child_work88(void *task, void *handler);
extern void *task_spawn_current_work6c(void *task, void *handler);
extern void *task_spawn_list_work7c(void *task, void *handler);
extern void *task_spawn_actor_work(void *task, void *handler);
extern void  actor_tick_scattered_projectile(void);                            /* child handler template */
extern void  release_slot_entry(void *entity);
extern void  mtx_load_abc(void);
extern void  mtx_apply_vec_at8(void *entity);
extern void  mtx_rotate_zyx_at24(void *entity);
extern void  model_submit_culled(void *entity, s32 flag);
extern void  collision_insert_grid(void *entity);

extern void *g_060906e8[];   /* per-task script pointer table (index = task->unkF) */
extern s32   g_060521f4[];   /* actor-slot parameter blocks (16-byte stride) */
extern s32   g_sound_effects_enabled;
extern u8    g_effect_slot_occupied_legacy[];
extern s32   g_matrix_stack_cursor;

static void copy_parent_state(void *child, void *ent)
{
    ACTOR_WORK(child)->position_x  = ACTOR_WORK(ent)->position_x;
    ACTOR_WORK(child)->position_y  = ACTOR_WORK(ent)->position_y;
    ACTOR_WORK(child)->position_z = ACTOR_WORK(ent)->position_z;
    ACTOR_WORK(child)->angle_x = ACTOR_WORK(ent)->angle_x;
    ACTOR_WORK(child)->angle_y = ACTOR_WORK(ent)->angle_y;
    ACTOR_WORK(child)->angle_z = ACTOR_WORK(ent)->angle_z;
    ACTOR_WORK(child)->movement_x = ACTOR_WORK(ent)->movement_x;
    ACTOR_WORK(child)->movement_y = ACTOR_WORK(ent)->movement_y;
    ACTOR_WORK(child)->movement_z = ACTOR_WORK(ent)->movement_z;
}

void actor_tick_scripted_spawner(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);
    void *script;

    if (state == 2) {
        release_slot_entry(ent);
        if (ACTOR_WORK(ent)->slot_id > 8) {
            g_effect_slot_occupied_legacy[ACTOR_WORK(ent)->slot_id - 8] = 0;
        }
        TASK_PTR(task, handler) = 0;
        return;
    }
    if (state != 0 && state != 1) {
        return;
    }

    script = g_060906e8[M8(task, 0xF)];

    if (state == 0) {
        s32 pbi = M32(script, 0x4);
        s32 *pb = &g_060521f4[pbi * 4];
        s8 slot;
        ACTOR_WORK(ent)->position_y += pb[1];
        ACTOR_WORK(ent)->movement_x = 0;
        ACTOR_WORK(ent)->movement_y = 0;
        ACTOR_WORK(ent)->movement_z = 0;
        M32(ent, 0x80) = 0;
        M32(ent, 0x7C) = 0;
        M32(ent, 0x78) = 0;
        M16(ent, 0x30) = (s16)M32(script, 0x8);
        M32(ent, 0xA8) = M32(script, 0xC) + 0x2000;
        slot = (s8)actor_allocate_effect_slot();
        ACTOR_WORK(ent)->slot_id = slot;
        if (slot < 0) {
            TASK_PTR(task, handler) = 0;
            return;
        }
        ACTOR_M32(ent, render_flags) = 0x100000;
        M32(ent, 0xBC) = 0x7FFFFF00;
        actor_register_movement_slot(ent, pb[0], pb[1], pb[2], pb[3]);
        ACTOR_M16(ent, motion_heading) = 0;
        TASK_S8(task, state) = 1;
    }

    {
        s32 flags = M32(script, 0x0);
        actor_resolve_movement(ent, (flags & 0x10000) ? 3 : 1);

        if (ACTOR_M8(ent, action_pending) != 0) {
            ACTOR_M8(ent, action_pending) = 0;
            if (ACTOR_M8(ent, hit_source_slot) < 8) {
                void *obj = 0;
                s32 idx;
                u8 *e;

                TASK_S8(task, state) = 2;
                direction_from_yaw_pitch(ACTOR_M16(ent, motion_pitch), ACTOR_M16(ent, motion_heading),
                              (s32 *)((u8 *)ent + 0x14),
                              (s32 *)((u8 *)ent + 0x18),
                              (s32 *)((u8 *)ent + 0x1C));
                ACTOR_WORK(ent)->movement_x = fixed_mul16(ACTOR_WORK(ent)->movement_x, M16(ent, 0x64) << 9);
                {
                    s32 a18 = ACTOR_WORK(ent)->movement_y;
                    if (a18 < 0) {
                        a18 = -a18;
                    }
                    ACTOR_WORK(ent)->movement_y = (a18 >> 3) + 0x1400;
                }
                ACTOR_WORK(ent)->movement_z = fixed_mul16(ACTOR_WORK(ent)->movement_z, M16(ent, 0x64) << 9);

                if ((flags & 0xFFFF) && g_sound_effects_enabled != 0) {
                    sound_queue_push((u32)(((flags & 0xFFFF) << 8) | DH_SOUND_CUE_BASE));
                }

                idx = 1;
                e = (u8 *)script;
                for (;;) {
                    s32 fe;
                    e += 16;
                    if (M32(e, 0) < 0) {
                        break;
                    }
                    fe = M32(e, 0);
                    if (fe & 2) {
                        if (effect_pool_make_room() != 0) {
                            void *o2 = effect_pool_spawn(M32(e, 4), 3, 0);
                            if (o2 != 0) {
                                ACTOR_M16(ent, motion_heading) += (s16)((game_random_next() & 0x1FFF) + 0xF000);
                                effect_pool_update_pose((s8)M8(MP(o2, 8), 0x34), 3, (u8 *)ent + 8, ent);
                            }
                        }
                    } else if (fe & 8) {
                        void *child;
                        switch (M32(e, 4)) {
                        case 0: obj = task_spawn_child_work88(task, MP(e, 8)); break;
                        case 1: obj = task_spawn_current_work6c(task, MP(e, 8)); break;
                        case 2: obj = task_spawn_list_work7c(task, MP(e, 8)); break;
                        case 3: obj = task_spawn_actor_work(task, MP(e, 8)); break;
                        default: break;
                        }
                        M8(task, 0xF) = (s8)idx;
                        child = MP(obj, 8);
                        M32(child, 0x3C) = M32(e, 0xC);
                        copy_parent_state(child, ent);
                        if (M32(e, 4) != 1) {
                            ACTOR_WORK(child)->slot_id = ACTOR_WORK(ent)->slot_id;
                        }
                    } else if ((fe & 4) == 0) {
                        void *child;
                        obj = task_spawn_child_work88(task, (void *)&actor_tick_scattered_projectile);
                        M8(task, 0xF) = (s8)idx;
                        child = MP(obj, 8);
                        copy_parent_state(child, ent);
                        ACTOR_WORK(child)->slot_id = ACTOR_WORK(ent)->slot_id;
                        M32(child, 0x60) = M32(e, 0xC);
                        M32(child, 0x64) = M32(e, 0x4);
                        M16(child, 0x30) = (s16)M32(e, 0x8);
                        M32(child, 0x80) = M32(e, 0x0);
                        M8(child, 0x36) = ACTOR_M8(ent, hit_source_slot);
                    }
                    idx++;
                }
            }
        }
    }

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
    collision_insert_grid(ent);
}

#include "dh_work.h"
#include "dh.h"
/* 0x0223d768 (effect_bounce_task) - bouncing debris/projectile effect task.
 *
 * ABI: task_handler(task). task = r4(arg0); entity = task->unk8 (r14).
 * Sibling of effect_fall_task (0x0223d1b6): shares the matrix/draw tail
 * (mtx_load_abc / mtx_apply_vec_at8 / mtx_rotate_zyx_at24 / func_02215a2c)
 * wrapped in the g_mtx_sp (0x06098878) push/pop of one 0x30-byte matrix frame.
 *
 * Per-frame state machine on task->unk10 (byte):
 *   state 0 (launch): advance position (unk8 += unk14*2), clear unk1C, arm the
 *       sprite (func_022173f4(entity, SPRITE_DESC_TBL, 2)); if the global object
 *       latch is engaged (g_play_active==1 && g_obj_latch>=2) reset the latched
 *       object (obj_latch_reset to level 2); state++ then fall into the draw tail.
 *   state 1 (fall/bounce): snapshot pos into unk4C/50/54, reload base velocity
 *       (unk14 = unk40). Near the floor (unkC <= 0x800) bounce unk70: if |unk70|
 *       is small (< 0x360) kick a fresh randomized up-speed ((rng()&0x7FF)+0x2400)
 *       and bias velocity (unk14 = unk40-0x600); else damp it (unk70 = -(unk70>>1)).
 *       Offset unk70 by -1296 through the softfloat lib (unk70 =
 *       dtoi(itod(unk70) - 1296.0)), copy to unk18, spin the angles
 *       (unk28 += 0x400, unk24 += 0x280), re-arm sprite frame 1. Then either:
 *         - on impact (g_collide_dir & 0xF) OR unk5D!=0: convert this task into
 *           effect_prescatter_task (task->unk0=0x0223101c, state=0), set burst
 *           params (unk30=0x187, unk32=0), emit a child sprite, run the obj latch
 *           (level 1) + fire a camera-shake (g_shake_flags|=1, latched obj unkAA=0xA),
 *           and tail-spawn an obj_anim_step object; return.
 *         - otherwise: drawlist_prepend(entity) and fall into the draw tail.
 *   default: just the draw tail.
 *
 * Branches/ptrs resolved via braf_resolve (all real TARGETs) + ptr_resolve
 * (mova+add: 0x0223101c effect_prescatter_task, 0x02238e94 obj_anim_step).
 * m2c correction: its `*M2C_ERROR($r2)=func_0224a7a8()` is entity->unk70 (r2 held
 * &entity->unk70 across the softfloat calls). Bounded 0x23d768..0x23d980 (1 rts).
 * Softfloat block 0x23d862..0x23d88c: stack = [&R][A=itod(unk70)][B=1296.0
 * (0x40944000:00000000)] -> dsub 0x0224a518 writes R = A - B (callee pops 20) ->
 * dtoi(R) -> unk70. The double words are passed to dsub explicitly (hi, lo).
 * fidelity: HIGH (softfloat call sequence hand-checked vs asm). */

#define G_PLAY_ACTIVE   DH_PLAY_SESSION_MODE_ADDR   /* s32, 1 = in-play                         */
#define G_OBJ_LATCH     DH_VIEW_REQUEST_PRIORITY_ADDR   /* s32 object-latch level                   */
#define G_LATCH_OBJ     DH_VIEW_TASK_ADDR   /* void** to the latched object             */
#define G_SHAKE_FLAGS   0x06098834   /* s32 camera-shake flag bits (bit0)        */
#define G_MTX_SP        DH_MATRIX_STACK_CURSOR_ADDR   /* s32 active matrix-stack pointer          */
#define G_COLLIDE_DIR   0x060B88A4   /* s32 collision direction bits (low nibble)*/
#define SPRITE_DESC_TBL 0x0608D3C8   /* sprite descriptor table                  */

#define GI32(a) (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern void mtx_load_abc(void);                       /* 0x022149aa */
extern void mtx_apply_vec_at8(void *entity);          /* 0x02214a40 */
extern void mtx_rotate_zyx_at24(void *entity);        /* 0x02214a06 */
extern void model_submit_culled(void *entity, s32 arg);     /* 0x02215a2c */
extern void collision_sweep_pose(void *entity, s32 tbl, s32 frame); /* 0x022173f4 */
extern void sprite_draw_child(void *entity, s32 a, s32 b); /* 0x022153f6 draw_child_sprite */
extern void spawn_obj_0x6c(void *task, void *handler);/* 0x0220fece */
extern void drawlist_prepend(void *entity);           /* 0x0222eea2 */
extern s32  game_random_next(void);                      /* 0x02210020 rng, no args */
extern double itod(s32 x);                            /* 0x0224a830 int->double */
extern void shc_double_subtract(u32 *result, u32 a_hi, u32 a_lo,
                          u32 b_hi, u32 b_lo);        /* 0x0224a518 dsub: *result = a - b */
extern s32  dtoi(double x);                           /* 0x0224a7a8 double->int  */

void effect_bounce_task(void *task)
{
    void *entity = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x8) = M32(entity, 0x8) + M32(entity, 0x14) * 2;
        M32(entity, 0x1C) = 0;
        collision_sweep_pose(entity, SPRITE_DESC_TBL, 2);
        if (GI32(G_PLAY_ACTIVE) == 1 && GI32(G_OBJ_LATCH) >= 2) {
            void *q = MP(GP(G_LATCH_OBJ), 8);
            GI32(G_OBJ_LATCH) = 2;
            M8(GP(G_LATCH_OBJ), 0xD) = 1;
            M16(q, 0xAE) = 0;
            M16(q, 0xB0) = 0;
            M16(q, 0xB2) = 0;
        }
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        goto draw_tail;                      /* fall through to the shared tail */

    case 1: {
        s32 v;
        M32(entity, 0x4C) = M32(entity, 0x8);
        M32(entity, 0x50) = M32(entity, 0xC);
        M32(entity, 0x54) = M32(entity, 0x10);
        M32(entity, 0x14) = M32(entity, 0x40);
        if (M32(entity, 0xC) <= 0x800) {
            v = M32(entity, 0x70);
            if ((v < 0 ? -v : v) < 0x360) {
                M32(entity, 0x70) = (game_random_next() & 0x7FF) + 0x2400;
                M32(entity, 0x14) = M32(entity, 0x40) - 0x600;
            } else {
                M32(entity, 0x70) = -(v >> 1);
            }
        }
        /* unk70 = dtoi(itod(unk70) - 1296.0) via the softfloat lib
         * (1296.0 == 0x40944000:00000000). */
        {
            union { double d; u64 u; } a, r;
            u32 res[2];                          /* big-endian: [0] = hi word */
            a.d = itod(M32(entity, 0x70));
            shc_double_subtract(res, (u32)(a.u >> 32), (u32)a.u, 0x40944000u, 0);
            r.u = ((u64)res[0] << 32) | res[1];
            M32(entity, 0x70) = dtoi(r.d);
        }
        M32(entity, 0x18) = M32(entity, 0x70);
        M16(entity, 0x28) = (s16)(M16(entity, 0x28) + 0x400);
        M16(entity, 0x24) = (s16)(M16(entity, 0x24) + 0x280);
        collision_sweep_pose(entity, SPRITE_DESC_TBL, 1);

        if ((GI32(G_COLLIDE_DIR) & 0xF) || M8(entity, 0x5D) != 0) {
            TASK_PTR(task, handler) = (void *)0x0223101c;   /* -> effect_prescatter_task */
            TASK_S8(task, state) = 0;
            M16(entity, 0x30) = 0x187;
            M16(entity, 0x32) = 0;
            sprite_draw_child(entity, 0x80000, 1);
            if (GI32(G_PLAY_ACTIVE) == 1 && GI32(G_OBJ_LATCH) >= 1) {
                void *q = MP(GP(G_LATCH_OBJ), 8);
                GI32(G_OBJ_LATCH) = 1;
                M8(GP(G_LATCH_OBJ), 0xD) = 1;
                M16(q, 0xAE) = 0;
                M16(q, 0xB0) = 0;
                M16(q, 0xB2) = 0;
            }
            if (GI32(G_PLAY_ACTIVE) == 1) {
                GI32(G_SHAKE_FLAGS) |= 1;
                M16(MP(GP(G_LATCH_OBJ), 8), 0xAA) = 0xA;
            }
            spawn_obj_0x6c(task, (void *)0x02238e94);   /* obj_anim_step (tail) */
            return;
        }
        drawlist_prepend(entity);
        goto draw_tail;
    }

    default:
        goto draw_tail;
    }

draw_tail:
    GI32(G_MTX_SP) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(entity);
    mtx_rotate_zyx_at24(entity);
    model_submit_culled(entity, 0);
    GI32(G_MTX_SP) -= 0x30;
}

/* evt_actor_fall_proximity_step [0x0221e8c8]: Existing map corroborated by source: fall + per-frame draw; despawn on player proximity */
#include "dh_work.h"
#include "dh.h"
/* 0x0221e8c8  evt_actor_fall_proximity_step - scripted-event actor step (task FSM)
 * with a per-frame draw pass. Resolves the first half of the previously-deferred
 * pool-contaminated cluster at 0x0221e8c4 (its real entry per find_entry.py; the
 * proximity helper it calls is func_0221e9c6). arg0 = task; task->unk8 = entity.
 *   state 0 (one-time init): seed the entity from the scene object g_0609882c:
 *     entity->unk8  = *0x06074efc;
 *     entity->unkC  = 0x2800;
 *     entity->unk10 = sceneobj->unk8->unk10;   (initial vertical position)
 *     entity->unk1C = -0x2000;                 (fall velocity)
 *     entity->unk28 = (s16)(sceneobj->unk8->unk10 >> 4);
 *     entity->unk30 = 64;  entity->unk32 = 90; (lifetime);  task->unk10++.
 *   every frame:
 *     if either active player (gate g_06098568 bits 0/1) is within proximity
 *       (func_0221e9c6 returns != 0) reset the lifetime entity->unk32 = 0;
 *     if (--entity->unk32 <= 0) { task->unk0 = 0; return; }  (terminate)
 *     integrate: entity->unk10 += entity->unk1C; entity->unk24 += 0x600;
 *       entity->unk26 += 0x400;
 *     draw inside the g_06098878 priority push/pop (+/- 0x30): mtx_load_abc,
 *       mtx_apply_vec_at0(&entity->unk8), mtx_rotate_zyx(&entity->unk24),
 *       mat3_scale_columns(0x20000,0x20000,0x20000), func_02215a2c(entity, 0).
 *
 * fidelity: HIGH, with ONE noted caveat: mtx_load_abc (mtx_load_abc) is reached with
 * a stale r4 (the asm sets up no argument for it; the matrix loader reads the cached
 * matrix g_06098abc and ignores r4) - modelled as (task) to match the sibling draw
 * step evt_actor_render (0x219916), arg value immaterial. The player-proximity gate
 * short-circuits exactly as the asm (p2 only tested when p1 is inactive or out of
 * range). bsrf targets resolved (braf_resolve.py). Matches m2c_fn.py --asm. */

#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern s32  evt_proximity_spawn(void *a, void *b);           /* proximity helper */
extern void mtx_load_abc(void *task);                 /* mtx_load_abc (arg unused) */
extern void mtx_apply_vec_at0(void *p);                    /* 0x214a50 */
extern void mtx_rotate_zyx(void *p);                    /* mtx_rotate_zyx */
extern void matrix_scale_columns(s32 x, s32 y, s32 z);        /* mat3_scale_columns */
extern void model_submit_culled(void *entity, s32 arg);      /* sprite submitter */
extern s32  g_matrix_stack_cursor;                                /* draw-priority accumulator */

void evt_actor_fall_proximity_step(void *task) {
    void *entity = TASK_PTR(task, work);

    if (TASK_S8(task, state) == 0) {
        void *so = MP(GP(DH_VIEW_TASK_ADDR), 0x8);
        M32(entity, 0x8)  = G32(0x06074efc);
        M32(entity, 0xC)  = 0x2800;
        M32(entity, 0x10) = M32(so, 0x10);
        M32(entity, 0x1C) = -0x2000;
        M16(entity, 0x28) = (s16)(M32(so, 0x10) >> 4);
        M16(entity, 0x30) = 64;
        M16(entity, 0x32) = 90;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }

    if (((G32(0x06098568) & 1) && evt_proximity_spawn(entity, MP(GP(0x060995b4), 0x8)) != 0) ||
        ((G32(0x06098568) & 2) && evt_proximity_spawn(entity, MP(GP(0x060995b8), 0x8)) != 0)) {
        M16(entity, 0x32) = 0;
    }

    {
        s16 old = M16(entity, 0x32);
        M16(entity, 0x32) = (s16)(old - 1);
        if (old <= 0) {
            TASK_S32(task, handler) = 0;
            return;
        }
    }

    M32(entity, 0x10) += M32(entity, 0x1C);
    M16(entity, 0x24) += 0x600;
    M16(entity, 0x26) += 0x400;

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc(task);                               /* r4 stale; loader ignores it */
    mtx_apply_vec_at0((void *)((u8 *)entity + 0x8));
    mtx_rotate_zyx((void *)((u8 *)entity + 0x24));
    matrix_scale_columns(0x20000, 0x20000, 0x20000);
    model_submit_culled(entity, 0);
    g_matrix_stack_cursor -= 0x30;
}

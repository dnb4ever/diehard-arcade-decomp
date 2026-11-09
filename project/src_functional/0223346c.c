/* actor_task_setup_dispatch [0x0223346c]: Existing map corroborated by source: entity=task->unk8. First call (task->unk10==0) inits actor: (src symbols_band23mid.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223346c - actor task spawn/setup + per-frame dispatch. entity = task->unk8.
 * On the first call (task->unk10 == 0) it initialises the actor: state unk34 = 4,
 * flag unk84 |= 0x80000000, unk5E = task->unkF, unk4F = task->unk12; runs
 * func_0222ca54(entity); seeds the variant word unk5C from table g_0608882e (stride
 * 8 by task->unk13); loads pose buffers (func_0222db44); registers an input slot
 * (func_0221707c, all-zero args); sets unkCE = (unk5E == 0x2E) ? 1 : 0; marks unk41
 * = 1 and bumps the init counter task->unk10. Every call: if entity->unk54 bit1 is
 * set, re-pick unk5C from the table using rng_next()&3; run animation_update
 * (actor_anim_frame_update); then tail-dispatch through the handler table
 * g_0609135c[entity->unkCE](entity).
 * fidelity: HIGH - all 5 bsrf resolved; final jsr is a RAM fn-ptr table dispatch.
 * The r5/r6/r7 arguments present in the prologue ABI are never read (single-arg). */
extern s16   g_0608882e[];                               /* variant table, stride 8 (4 words) */
extern void (*g_0609135c[])(void *entity);               /* per-type handler table */
extern void  actor_load_part_ids(void *entity);
extern void  actor_load_pose_buffers(void *entity, s32 variant);   /* actor_load_pose_buffers */
extern void  actor_register_movement_slot(void *entity, s32 a, s32 b, s32 c, s32 d);  /* actor_slot_record_reg */
extern s32   game_random_next(void);                        /* rng_next */
extern void  actor_anim_frame_update(void *task, void *entity);    /* animation_update */

void actor_task_setup_dispatch(void *task) {
    void *entity = TASK_PTR(task, work);

    if (TASK_S8(task, state) == 0) {
        M8(entity, 0x34) = 4;
        M32(entity, 0x84) |= (s32)0x80000000;
        M16(entity, 0x5E) = M8(task, 0xF);
        M8(entity, 0x4F) = M8(task, 0x12);
        actor_load_part_ids(entity);
        M16(entity, 0x5C) = g_0608882e[M8(task, 0x13) * 4];
        actor_load_pose_buffers(entity, M16(entity, 0x5C));
        actor_register_movement_slot(entity, 0, 0, 0, 0);
        M16(entity, 0xCE) = (M16(entity, 0x5E) == 0x2E) ? 1 : 0;
        M16(entity, 0x5C) = g_0608882e[M8(task, 0x13) * 4];
        M8(entity, 0x41) = 1;
        TASK_S8(task, state) += 1;
    }
    if (MU16(entity, 0x54) & 2)
        M16(entity, 0x5C) = g_0608882e[M8(task, 0x13) * 4 + (game_random_next() & 3)];
    actor_anim_frame_update(task, entity);
    g_0609135c[M16(entity, 0xCE)](entity);
}

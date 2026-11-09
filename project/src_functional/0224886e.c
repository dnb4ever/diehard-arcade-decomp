/* enemy_update_carry_throw [0x0224886e]: Synchronizes partner pose and dispatches carry/throw action commands. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224886e  actor carry/throw handler (3-state, with an 0x88-command
 * sub-dispatch). ABI (r4=task, r5=entity). animation_update (0x0222dc2c) is called
 * through a register (jsr @r12) mid-body and as some tails.
 *
 *   switch (entity[0xd8]) {
 *     case 0:
 *       animation_update(task, entity);
 *       partner = g_060995B0[entity[0x4b]]->unk8;
 *       dx = ((u16)partner[0x26] == 0x4000) ? -0x1000 : 0x1000;
 *       if (entity[0x54] & 2) { partner[0x8] = entity[0x8] + dx; entity[0xd8]=1; }
 *       else partner[0x8] += (entity[0x8] + dx - partner[0x8]) >> 1;  (ease)
 *       return;
 *     case 1: func_022490a0(entity);        (falls through to case 2)
 *     case 2:
 *       if (entity[0x4b] == 0) {
 *         entity[0xbc] = entity[0x140];
 *         switch (entity[0x88]) {
 *           0x45: func_02249240(entity); return;                      (tail)
 *           0x48: func_022499a6(entity); animation_update; return;
 *           0x47: entity[0x44]^=1; entity[0x4c]=entity[0x67];
 *                 entity[0xd4]=0xc; entity[0xd8]=1; entity[0xbc]=-1;
 *                 entity[0x84]=0x40000; animation_update; return;
 *           0x49: entity[0x4a]=entity[0x34]; entity[0xd4]=0x11;
 *                 entity[0xd8]=func_0223ffcc(task,entity); return;
 *           0x46/default: func_022469f2(entity); animation_update; return;
 *         }
 *       }
 *       return;
 *   }
 *
 * func_022499a6 / func_02249240 / func_022490a0 are sibling handlers in this unit;
 * func_0223ffcc is (task,entity)->new-state. All computed branches resolved.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void enemy_spawn_clash_task(void *entity);                 /* 0x022490a0 */
extern void enemy_enter_timed_reaction(void *entity);                 /* 0x02249240 */
extern void enemy_enter_optional_pose(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern s32  actor_dispatch_action_and_finish(void *task, void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_carry_throw(void *task, void *entity) {
    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0: {
        void *partner;
        s32 dx;
        actor_anim_frame_update(task, entity);
        partner = TASK_PTR(((void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR)[ACTOR_M8(entity, paired_slot)], work);
        dx = ((u16)M16(partner, 0x26) == 0x4000) ? -0x1000 : 0x1000;
        if (ACTOR_MU16(entity, animation_events) & 2) {
            M32(partner, 0x8) = ACTOR_M32(entity, position_x) + dx;
            ACTOR_M32(entity, behavior_phase) = 1;
        } else {
            M32(partner, 0x8) += (ACTOR_M32(entity, position_x) + dx - M32(partner, 0x8)) >> 1;
        }
        return;
    }
    case 1:
        enemy_spawn_clash_task(entity);
        /* fallthrough */
    case 2:
        if (ACTOR_M8(entity, paired_slot) == 0) {
            M32(entity, 0xBC) = M32(entity, 0x140);
            switch (ACTOR_M32(entity, action_state)) {
            case 0x45:
                enemy_enter_timed_reaction(entity);
                return;
            case 0x48:
                enemy_enter_optional_pose(entity);
                actor_anim_frame_update(task, entity);
                return;
            case 0x47:
                ACTOR_M8(entity, facing_right) ^= 1;
                ACTOR_M8(entity, hit_pending) = M8(entity, 0x67);
                ACTOR_M32(entity, behavior_state) = 0xC;
                ACTOR_M32(entity, behavior_phase) = 1;
                M32(entity, 0xBC) = -1;
                ACTOR_M32(entity, action_category) = 0x40000;
                actor_anim_frame_update(task, entity);
                return;
            case 0x49:
                ACTOR_M8(entity, hit_source_slot) = ACTOR_M8(entity, slot_id);
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
                ACTOR_M32(entity, behavior_phase) = actor_dispatch_action_and_finish(task, entity);
                return;
            case 0x46:
            default:
                enemy_reset_idle_or_walk(entity);
                actor_anim_frame_update(task, entity);
                return;
            }
        }
        return;
    }
}

#include "dh_work.h"
/* enemy_update_held_commands [0x02248f88]: Runs the held-command phases with hit and lifetime gates before animation. */
#include "dh.h"
/* 0x02248f88  actor command/dispatch handler (2-state, with an 0x88-command
 * sub-dispatch). ABI (r4=task, r5=entity). Calls animation_update (0x0222dc2c)
 * through a register (jsr @r12) mid-body and as some tails.
 *
 *   switch (entity[0xd8]) {
 *     case 0:
 *       animation_update(task, entity);
 *       if (entity[0x54] & 2) { entity[0x5c]=0xdd; entity[0xd8]=1; }
 *       return;
 *     case 1:
 *       if (entity[0x4c] != 0) return;              (hit pending -> wait)
 *       if (entity[0x56] <= 0) { func_02249cda(task, entity); return; }  (timer out)
 *       entity[0xbc] = entity[0x140];
 *       switch (entity[0x88]) {                     (queued command)
 *         0x45/0x46: func_02227704(entity); -> (default)
 *         default:   func_022469f2(entity); animation_update(task,entity); return;
 *         0x48: func_02249240(entity); return;        (tail)
 *         0x47: entity[0x44]^=1; entity[0x4b]=entity[0x67];
 *               animation_update(task,entity); entity[0x84]=0x20000;
 *               func_022490a0(entity); return;        (tail)
 *         0x49: entity[0x4a]=entity[0x34]; entity[0xd4]=0x11;
 *               entity[0xd8]=func_0223ffcc(task,entity); return;
 *       }
 *   }
 *
 * func_02227704 is 1-arg (verified: its prologue reads only r4). func_0223ffcc is
 * (task,entity)->new-state as in 0x24852c. func_02249cda/func_02249240/func_022490a0 are
 * sibling handlers in this unit. All computed branches resolved. fidelity: HIGH -
 * hand-traced vs m2c_fn --asm. */
extern void actor_resolve_facing(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern s32  actor_dispatch_action_and_finish(void *task, void *entity);
extern void enemy_enter_despawn(void *task, void *entity);     /* 0x02249cda */
extern void enemy_enter_timed_reaction(void *entity);                 /* 0x02249240 */
extern void enemy_spawn_clash_task(void *entity);                 /* 0x022490a0 */
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_held_commands(void *task, void *entity) {
    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0:
        actor_anim_frame_update(task, entity);
        if (ACTOR_MU16(entity, animation_events) & 2) {
            ACTOR_MU16(entity, animation_requested) = DH_ANIM_HELD_READY;
            ACTOR_M32(entity, behavior_phase) = 1;
        }
        return;
    case 1:
        if (ACTOR_M8(entity, hit_pending) != 0) {
            return;
        }
        if (ACTOR_M16(entity, health) <= 0) {
            enemy_enter_despawn(task, entity);
            return;
        }
        M32(entity, 0xBC) = M32(entity, 0x140);
        switch (ACTOR_M32(entity, action_state)) {
        case 0x45:
        case 0x46:
            actor_resolve_facing(entity);
            /* fallthrough */
        default:
            enemy_reset_idle_or_walk(entity);
            actor_anim_frame_update(task, entity);
            return;
        case 0x48:
            enemy_enter_timed_reaction(entity);
            return;
        case 0x47:
            ACTOR_M8(entity, facing_right) ^= 1;
            ACTOR_M8(entity, paired_slot) = M8(entity, 0x67);
            actor_anim_frame_update(task, entity);
            ACTOR_M32(entity, action_category) = 0x20000;
            enemy_spawn_clash_task(entity);
            return;
        case 0x49:
            ACTOR_M8(entity, hit_source_slot) = ACTOR_M8(entity, slot_id);
            ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
            ACTOR_M32(entity, behavior_phase) = actor_dispatch_action_and_finish(task, entity);
            return;
        }
    }
}

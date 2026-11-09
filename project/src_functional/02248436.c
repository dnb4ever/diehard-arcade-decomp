/* enemy_update_turn [0x02248436]: Waits for animation readiness, flips facing/yaw and completes the turn state. */
#include "dh.h"
#include "dh_work.h"
/* 0x02248436  actor turn/flip FSM, then tail to animation_update (0x0222dc2c).
 * ABI (r4=task, r5=entity).
 *   if entity[0x4c] != 0 -> func_022489d0(entity)   (hit/stun branch)
 *   else switch (entity[0xd8]):
 *     case 0: if (entity[0x54]&2) entity[0xe0]|=0x10000; entity[0xd8]=1; (fall)
 *     case 1: if (entity[0x54]&2) {                 // ready to flip
 *               entity[0xd8]=2; entity[0x44]^=1; entity[0x26]+=0x8000;
 *               entity[0x84]=1; entity[0x88]=0;
 *               entity[0xe0]&=0x300; entity[0xe0]|=0x20000;
 *               entity[0x108]=0; entity[0xbc]=entity[0x140];
 *               entity[0x41]=1; entity[0x5c]=(s16)entity[0x150]; }
 *             break;
 *     case 2: if (func_02245eb8(entity,2)) func_022499a6(entity);
 *             else func_02246cac(entity);  break;
 *   always: animation_update(task, entity)   [braf tail, resolved].
 * All 4 computed branches resolved. fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void actor_enter_hit_reaction(void *entity);
extern s32  enemy_test_player_pair_lock(void *entity, s32 arg);
extern void enemy_enter_optional_pose(void *entity);
extern void enemy_begin_approach(void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_turn(void *task, void *entity) {
    if (ACTOR_M8(entity, hit_pending) != 0) {
        actor_enter_hit_reaction(entity);
    } else {
        switch (ACTOR_M32(entity, behavior_phase)) {
        case 0:
            if (ACTOR_MU16(entity, animation_events) & 2) {
                ACTOR_WORK(entity)->state_flags |= 0x10000;
            }
            ACTOR_M32(entity, behavior_phase) = 1;
            /* fallthrough */
        case 1:
            if (ACTOR_MU16(entity, animation_events) & 2) {
                ACTOR_M32(entity, behavior_phase) = 2;
                ACTOR_WORK(entity)->facing_right ^= 1;
                ACTOR_WORK(entity)->angle_y = (s16)(ACTOR_WORK(entity)->angle_y + 0x8000);
                ACTOR_WORK(entity)->action_category = 1;
                ACTOR_WORK(entity)->action_state = 0;
                ACTOR_WORK(entity)->state_flags &= 0x300;
                ACTOR_WORK(entity)->state_flags |= 0x20000;
                ACTOR_M32(entity, action_phase) = 0;
                M32(entity, 0xBC) = M32(entity, 0x140);
                ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(entity)->animation_requested = M16(entity, 0x150);
            }
            break;
        case 2:
            if (enemy_test_player_pair_lock(entity, 2) != 0) {
                enemy_enter_optional_pose(entity);
            } else {
                enemy_begin_approach(entity);
            }
            break;
        default:
            break;
        }
    }
    actor_anim_frame_update(task, entity);
}

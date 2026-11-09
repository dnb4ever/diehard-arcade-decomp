#include "dh_work.h"
/* enemy_enter_dash_recovery_b [0x02247a3e]: Loads the recovery animation and enters state five/substate six. */
#include "dh.h"
/* 0x02247a3e  enter dash-recovery variant B: run func_0223ac50(actor), then move
 * to state unkD4=5 / sub-state unkD8=6, speed cap unk84=1, clear unk88.
 * bounds: [0x02247a3e, 0x247a66); split out of func_022473ee's over-swallowed
 *   extent. fidelity: HIGH - verified vs fn.py --asm. */
void actor_load_recovery_pose(void *actor);                 /* 0x0223ac50 */

void enemy_enter_dash_recovery_b(void *actor) {
    actor_load_recovery_pose(actor);
    ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_RECOVERY;
    ACTOR_M32(actor, behavior_phase) = 6;
    ACTOR_M32(actor, action_category) = 1;
    ACTOR_M32(actor, action_state) = 0;
}

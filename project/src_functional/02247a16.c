#include "dh_work.h"
/* enemy_enter_dash_recovery_a [0x02247a16]: Resets action state and enters dash recovery state five/substate three. */
#include "dh.h"
/* 0x02247a16  enter dash-recovery variant A: run func_0223afc4(actor), then set
 * speed cap unk84=1, clear unk88, and move to state unkD4=5 / sub-state unkD8=3.
 * bounds: [0x02247a16, 0x247a3e); split out of func_022473ee's over-swallowed
 *   extent. fidelity: HIGH - verified vs fn.py --asm. */
void actor_load_hit_pose(void *actor);                 /* 0x0223afc4 */

void enemy_enter_dash_recovery_a(void *actor) {
    actor_load_hit_pose(actor);
    ACTOR_M32(actor, action_category) = 1;
    ACTOR_M32(actor, action_state) = 0;
    ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_RECOVERY;
    ACTOR_M32(actor, behavior_phase) = 3;
}

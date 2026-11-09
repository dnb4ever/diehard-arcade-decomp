#include "dh_work.h"
/* enemy_restore_saved_pose [0x02242226]: Clears pending, restores XYZ from +ec/+f0/+f4, repositions and requests 0x470. */
#include "dh.h"
/* 0x02242226  snap an actor to its "home" pose: clear the busy flag (unk48),
 * copy the home coordinates unkEC/F0/F4 into the live coords unk8/C/10, set the
 * reposition bits in unkE0, and if the actor has taken enough hits (unk56 > 4)
 * run actor_apply_damage(actor, 4). Finally force facing (unk41=1) and heading
 * unk5C = 0x470.
 *
 * bounds: [0x242226, 0x242272); split out of func_02241eb0's over-swallowed
 *   0x3c2 extent (prologue follows the braf-tail rts at 0x242224).
 * fidelity: HIGH - straight-line; unk56 compare is cmp/gt (signed s16 > 4). */
void actor_apply_damage(void *actor, s32 arg);         /* 0x0222ab18 */

void enemy_restore_saved_pose(void *actor) {
    ACTOR_M8(actor, action_pending) = 0;
    ACTOR_M32(actor, position_x) = M32(actor, 0xEC);
    ACTOR_M32(actor, position_y) = M32(actor, 0xF0);
    ACTOR_M32(actor, position_z) = M32(actor, 0xF4);
    ACTOR_M32(actor, state_flags) |= 0x400400;
    if (ACTOR_M16(actor, health) > 4) {
        actor_apply_damage(actor, 4);
    }
    ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(actor, animation_requested) = 0x470;
}

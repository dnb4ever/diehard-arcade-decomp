#include "dh_work.h"
/* actor_set_state_7 [0x0224840a]: Enters state seven, enables the update flag and resets the action phase. */
#include "dh.h"
/* 0x0224840a  actor_set_state_7: field[0xd4]=7; field[0xd8]=0; field[0xe0]|=0x10000;
 * field[0x84]|=0x10; field[0x108]=0; field[0x41]=1.
 * fidelity: HIGH - exact. */
void actor_set_state_7(void *p) {
    ACTOR_M32(p, behavior_state) = DH_ENEMY_BEHAVIOR_TURN;
    ACTOR_M32(p, behavior_phase) = 0;
    ACTOR_M32(p, state_flags) |= 0x10000;
    ACTOR_M32(p, action_category) |= 0x10;
    ACTOR_M32(p, action_phase) = 0;
    ACTOR_M8(p, animation_control) = DH_ANIMATION_CONTROL_RESTART;
}

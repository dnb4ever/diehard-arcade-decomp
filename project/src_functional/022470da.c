#include "dh_work.h"
/* enemy_enter_state2 [0x022470da]: Enters state two and requests the action descriptor's pose at 0x14c. */
#include "dh.h"
/* 0x022470da  set actor to a state: field[0xd4]=2; field[0xd8]=0; field[0x84]=0x80;
 * field[0xe0] &= 0x300; field[0x41]=1; field[0x5c]=field[0x14c].
 * fidelity: HIGH - exact. */
void enemy_enter_state2(void *p) {
    ACTOR_M32(p, behavior_state) = 2;
    ACTOR_M32(p, behavior_phase) = 0;
    ACTOR_M32(p, action_category) = 0x80;
    ACTOR_M32(p, state_flags) &= 0x300;
    ACTOR_M8(p, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(p, animation_requested) = ACTOR_M16(p, state2_animation);
}

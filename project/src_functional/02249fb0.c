#include "dh_work.h"
/* enemy_enter_state19 [0x02249fb0]: Sets state nineteen, clears local action state and retains only flag bits eight and nine. */
#include "dh.h"
/* 0x02249fb0  enter actor state 19: field[0xd4]=19; field[0x84]=1; field[0x88]=0;
 * field[0xe0] &= 0x300.
 * fidelity: HIGH - exact. */
void enemy_enter_state19(void *p) {
    ACTOR_M32(p, behavior_state) = DH_ENEMY_BEHAVIOR_STOP;
    ACTOR_M32(p, action_category) = 1;
    ACTOR_M32(p, action_state) = 0;
    ACTOR_M32(p, state_flags) &= 0x300;
}

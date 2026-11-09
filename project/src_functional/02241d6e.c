/* enemy_enter_timed_state16 [0x02241d6e]: Behavior 16/phase 0 and 300-tick timer; primes animation 0x30b and flags. */
#include "dh_work.h"
#include "dh.h"
/* 0x02241d6e - func_02241d6e(entity): enter actor state 0x10. Primes the state
 * fields and copies the 0x148 timer into the 0x5C countdown:
 *   entity->0xD4 = 0x10; entity->0xD8 = 0;
 *   entity->0xC4 = 0x12C;
 *   entity->0x84 = 0x00800201;
 *   entity->0xE0 = 0x01200000;
 *   entity->0x4D |= 1;
 *   entity->0x41 = 1;
 *   entity->0x148 = 0x030B;
 *   entity->0x5C = entity->0x148;            // = 0x030B
 * fidelity: HIGH - straight-line stores, no calls. 0xD4/0xD8/0xC4/0x84/0xE0
 * are s32; 0x4D/0x41 bytes; 0x148/0x5C u16. The final +0x5C store reloads the
 * just-written +0x148 halfword (mov.w @(0x148) then store), modeled as such. */
void enemy_enter_timed_state16(void *entity)
{
    ACTOR_M32(entity, behavior_state) = 0x10;
    ACTOR_M32(entity, behavior_phase) = 0;
    M32(entity, 0xC4) = 0x12C;
    ACTOR_M32(entity, action_category) = 0x00800201;
    ACTOR_M32(entity, state_flags) = 0x01200000;
    ACTOR_M8(entity, direction_code) |= 1;
    ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(entity, idle_animation) = 0x030B;
    ACTOR_M16(entity, animation_requested) = ACTOR_M16(entity, idle_animation);
}

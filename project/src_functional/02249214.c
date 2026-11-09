#include "dh_work.h"
/* enemy_enter_state8 [0x02249214]: Enters state eight and requests the descriptor pose at 0x14e. */
#include "dh.h"
/* 0x02249214 - func_02249214(entity): enter actor state 8. Primes the state
 * fields and copies the 0x14E timer into the 0x5C countdown:
 *   entity->0xD4 = 8; entity->0xD8 = 0;
 *   entity->0x84 = 0x400;
 *   entity->0xE0 &= 0x1300;
 *   entity->0x41 = 1;
 *   entity->0x5C = entity->0x14E;
 * fidelity: HIGH - straight-line stores, no calls. 0xD4/0xD8/0x84/0xE0 s32;
 * 0x41 byte; 0x5C/0x14E u16. 0xE0 is read-modify-write masked with 0x1300. */
void enemy_enter_state8(void *entity)
{
    ACTOR_M32(entity, behavior_state) = 8;
    ACTOR_M32(entity, behavior_phase) = 0;
    ACTOR_M32(entity, action_category) = 0x400;
    ACTOR_M32(entity, state_flags) &= 0x1300;
    ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(entity, animation_requested) = ACTOR_M16(entity, state8_animation);
}

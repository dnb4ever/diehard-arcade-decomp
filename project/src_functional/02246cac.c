#include "dh_work.h"
/* enemy_begin_approach [0x02246cac]: Initializes approach state and selects pose or fallback from the target sweep result. */
#include "dh.h"
/* 0x02246cac - state-init helper (func_02246cac), called 3x from func_02246ab8.
 *   obj->0xD4 = 1; obj->0xD8 = 0; obj->0x84 = 4;
 *   obj->0xE0 = obj->0xE0 & 0x00021030;   (store 1, 0x246ccc)
 *   obj->0xE0 = obj->0xE0 | 2;            (re-read, store 2, 0x246cd0-0x246cd4)
 *   switch (func_02243fe0(obj)) {
 *     case 1: actor_set_state_7(obj); obj->0x5C = obj->0x14A; break;
 *     case 2: func_02249fb0(obj); return;      (braf tail, delta 0x32aa)
 *     default: break;
 *   }
 * fidelity: HIGH - the AND and OR on obj->0xE0 are two separate memory stores
 * (with a re-read in between), matching the write trace that observed the
 * intermediate AND-only value 0x30 before the OR of 2. The switch selector is
 * func_02243fe0's return value (m2c lost the register); 0x5C/0x14A are 16-bit.
 * No tst #imm8 occurs. */
s32  enemy_sweep_toward_target(void *obj);   /* extern (switch selector) */
void actor_set_state_7(void *obj);   /* extern */
void enemy_enter_state19(void *obj);   /* extern (case-2 tail) */

void enemy_begin_approach(void *obj)
{
    ACTOR_M32(obj, behavior_state) = 1;
    ACTOR_M32(obj, behavior_phase) = 0;
    ACTOR_M32(obj, action_category) = 4;
    ACTOR_M32(obj, state_flags) = ACTOR_M32(obj, state_flags) & 0x00021030;
    ACTOR_M32(obj, state_flags) = ACTOR_M32(obj, state_flags) | 2;

    switch (enemy_sweep_toward_target(obj)) {
    case 1:
        actor_set_state_7(obj);
        ACTOR_M16(obj, animation_requested) = ACTOR_M16(obj, approach_animation);
        break;
    case 2:
        enemy_enter_state19(obj);
        return;
    default:
        break;
    }
}

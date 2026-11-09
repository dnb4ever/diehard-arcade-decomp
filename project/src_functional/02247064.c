#include "dh_work.h"
/* enemy_enter_idle_state18 [0x02247064]: Enters state eighteen and requests the descriptor idle pose when it differs. */
#include "dh.h"
/* 0x02247064 - leaf state handler (func_02247064). No calls.
 *   obj->0xD4 = 0x12; obj->0x84 = 1; obj->0x88 = 0;
 *   obj->0xE0 &= 0x00020320; obj->0xE0 |= 1;   (two separate RMW stores)
 *   if (obj->0x5A != obj->0x148) { obj->0x41 = 1; obj->0x5C = obj->0x148; }
 * fidelity: HIGH - exact; 0x5A/0x148/0x5C are 16-bit; the compare is on raw
 * 16-bit values (equality, sign irrelevant). */
void enemy_enter_idle_state18(void *obj)
{
    ACTOR_M32(obj, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
    ACTOR_M32(obj, action_category) = 1;
    ACTOR_M32(obj, action_state) = 0;
    ACTOR_M32(obj, state_flags) = ACTOR_M32(obj, state_flags) & 0x00020320;   /* stored, then re-read */
    ACTOR_M32(obj, state_flags) = ACTOR_M32(obj, state_flags) | 1;
    if (ACTOR_M16(obj, animation_current) != ACTOR_M16(obj, idle_animation)) {
        ACTOR_M8(obj, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_M16(obj, animation_requested) = ACTOR_M16(obj, idle_animation);
    }
}

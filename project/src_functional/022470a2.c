#include "dh_work.h"
/* enemy_enter_descriptor_state18 [0x022470a2]: Enters state eighteen and requests the descriptor's animation at offset twelve. */
#include "dh.h"
/* 0x022470a2 - case-3 leaf handler (func_022470a2). Leaf, no calls.
 *   obj->0xD4 = 0x12; obj->0x84 = 1; obj->0x88 = 0;
 *   obj->0xE0 &= 0x00020030; (stored) then re-read and |= 2 (second store);
 *   obj->0x41 = 1;
 *   obj->0x5C = *(s16*)( *(void**)(*(void**)(obj+0x158)) + 0xC );
 * fidelity: HIGH - exact; two separate RMW stores on 0xE0 as in the ROM
 * (2470bc..2470c8), matching the documented pattern in func_02247064. */
void enemy_enter_descriptor_state18(void *obj)
{
    void *p = ACTOR_MP(obj, action_config_tables);
    ACTOR_M32(obj, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
    ACTOR_M32(obj, action_category) = 1;
    ACTOR_M32(obj, action_state) = 0;
    ACTOR_M32(obj, state_flags) = ACTOR_M32(obj, state_flags) & 0x00020030;
    ACTOR_M32(obj, state_flags) = ACTOR_M32(obj, state_flags) | 2;
    ACTOR_M8(obj, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(obj, animation_requested) = M16(MP(p, 0), 0xC);
}

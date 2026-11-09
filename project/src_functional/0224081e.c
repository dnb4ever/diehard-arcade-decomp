/* enemy_action_dispatch_special_state [0x0224081e]: State indices 9/13 and flag gates choose expire or facing-resolution helper. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224081e - func_0224081e: per-state dispatch wrapper (blob sibling of
 * func_022402c0). real fn [0x24081e, 0x24086c) - bounded by the rts at 0x240868.
 *   if (M32(obj,0xE4) == 9) { func_022402c0(obj, out); return; }
 *   if (M32(obj,0xE4) == 13 && (M32(obj,0xE0) & 3) &&
 *       !(M32(obj,0xE0) & 0x2010000)) {
 *       func_022408a0(obj, out);
 *       M32(obj,0x108) = 6;
 *       return;
 *   }
 *   func_02241164(obj, out);                        // (bra tail)
 * fidelity: HIGH - matches m2c exactly. 0xE4/0xE0 read mov.l; tst #3 and the
 * reg-form tst of 0x2010000 both decoded as plain masks (no sign-extend trap). */
extern void actor_check_expiry_or_lock(void *obj, s32 *out);    /* 0x022402c0 (tail) */
extern void enemy_action_resolve_descriptor_facing(void *obj, s32 *out);   /* 0x022408a0 */
extern void actor_select_walk_or_random_pose(void *obj, s32 *out);   /* 0x02241164 (tail) */

void enemy_action_dispatch_special_state(void *obj, s32 *out)
{
    if (ACTOR_M32(obj, state_index) == 9) {
        actor_check_expiry_or_lock(obj, out);
        return;
    }
    if (ACTOR_M32(obj, state_index) == 13 && (ACTOR_M32(obj, state_flags) & 3) &&
        !(ACTOR_M32(obj, state_flags) & 0x02010000)) {
        enemy_action_resolve_descriptor_facing(obj, out);
        ACTOR_M32(obj, action_phase) = 6;
        return;
    }
    actor_select_walk_or_random_pose(obj, out);
}

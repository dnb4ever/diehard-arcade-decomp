/* enemy_action_dispatch_special_state_signalled [0x0224086c]: Sets effect-slot completion word 060e0f2c[actor+94], then delegates to enemy_action_dispatch_special_state (0224081e). */
#include "dh.h"
/* 0x0224086c - func_0224086c: leaf flag-setter (blob sibling of func_022402c0).
 * real fn [0x0224086c, +0x10) - no stack frame; sets the per-actor flag then
 * tail-branches to func_0224081e with both args passed through unchanged.
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_0224081e(obj, out);                      // (bra tail)
 * fidelity: HIGH - exact. index M32(obj,0x94) scales the 4-byte table (shll2). */
extern void enemy_action_dispatch_special_state(void *obj, s32 *out);     /* 0x0224081e (tail) */
extern s32  g_060e0f2c[];                         /* s32 flag table, idx obj->0x94 */

void enemy_action_dispatch_special_state_signalled(void *obj, s32 *out)
{
    g_060e0f2c[M32(obj, 0x94)] = 1;
    enemy_action_dispatch_special_state(obj, out);
}

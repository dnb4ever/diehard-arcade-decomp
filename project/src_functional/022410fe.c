/* enemy_action_walk_or_face6_signalled [0x022410fe]: Sets effect-slot completion word 060e0f2c[actor+94], then delegates to enemy_action_walk_or_face6 (02241034). */
#include "dh.h"
/* 0x022410fe - func_022410fe: leaf flag-setter (blob sibling of func_022402c0).
 * real fn [0x022410fe, +0x10) - no stack frame; sets the per-actor flag then
 * tail-branches to func_02241034 with both args passed through unchanged.
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_02241034(obj, out);                      // (bra tail)
 * fidelity: HIGH - exact. index M32(obj,0x94) scales the 4-byte table (shll2). */
extern void enemy_action_walk_or_face6(void *obj, s32 *out);     /* 0x02241034 (tail) */
extern s32  g_060e0f2c[];                         /* s32 flag table, idx obj->0x94 */

void enemy_action_walk_or_face6_signalled(void *obj, s32 *out)
{
    g_060e0f2c[M32(obj, 0x94)] = 1;
    enemy_action_walk_or_face6(obj, out);
}

/* enemy_action_walk_or_face_alternate_signalled [0x02240fc8]: Sets effect-slot completion word 060e0f2c[actor+94], then delegates to enemy_action_walk_or_face_alternate (02240efe). */
#include "dh.h"
/* 0x02240fc8 - func_02240fc8: leaf flag-setter (blob sibling of func_022402c0).
 * real fn [0x02240fc8, +0x10) - no stack frame; sets the per-actor flag then
 * tail-branches to func_02240efe with both args passed through unchanged.
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_02240efe(obj, out);                      // (bra tail)
 * fidelity: HIGH - exact. index M32(obj,0x94) scales the 4-byte table (shll2). */
extern void enemy_action_walk_or_face_alternate(void *obj, s32 *out);     /* 0x02240efe (tail) */
extern s32  g_060e0f2c[];                         /* s32 flag table, idx obj->0x94 */

void enemy_action_walk_or_face_alternate_signalled(void *obj, s32 *out)
{
    g_060e0f2c[M32(obj, 0x94)] = 1;
    enemy_action_walk_or_face_alternate(obj, out);
}

/* enemy_action_walk_or_face_variant_signalled [0x02240eee]: Sets effect-slot completion word 060e0f2c[actor+94], then delegates to enemy_action_walk_or_face_variant (02240e54). */
#include "dh.h"
/* 0x02240eee - func_02240eee: leaf flag-setter (blob sibling of func_022402c0).
 * real fn [0x02240eee, +0x10) - no stack frame; sets the per-actor flag then
 * tail-branches to func_02240e54 with both args passed through unchanged.
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_02240e54(obj, out);                      // (bra tail)
 * fidelity: HIGH - exact. index M32(obj,0x94) scales the 4-byte table (shll2). */
extern void enemy_action_walk_or_face_variant(void *obj, s32 *out);     /* 0x02240e54 (tail) */
extern s32  g_060e0f2c[];                         /* s32 flag table, idx obj->0x94 */

void enemy_action_walk_or_face_variant_signalled(void *obj, s32 *out)
{
    g_060e0f2c[M32(obj, 0x94)] = 1;
    enemy_action_walk_or_face_variant(obj, out);
}

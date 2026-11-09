/* enemy_action_signal_direction_face [0x02240cca]: Damage/update, effect-slot signal and direction-based response. */
#include "dh.h"
/* 0x02240cca - func_02240cca: per-state actor setup + flag (blob sibling of
 * func_022402c0). real fn [0x240cca, 0x240cf6) - entry two scratch loads before
 * the push; bounded at the bra-tail to func_022414ee (0x240cf2).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_022414ee(obj, out);                        // facing-init tail
 * fidelity: HIGH - matches m2c exactly (func_02240b2e pattern, func_022414ee tail). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_enter_lateral_reaction(void *obj, s32 *out);      /* 0x022414ee (tail) */
extern s32  g_060e0f2c[];                             /* s32 flag table, idx obj->0x94 */

void enemy_action_signal_direction_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    g_060e0f2c[M32(obj, 0x94)] = 1;
    actor_enter_lateral_reaction(obj, out);
}

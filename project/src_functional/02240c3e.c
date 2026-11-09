/* enemy_action_signal_random_face [0x02240c3e]: Damage/update then effect-slot signal and random-angle response. */
#include "dh.h"
/* 0x02240c3e - func_02240c3e: per-state actor setup + flag (blob sibling of
 * func_022402c0). real fn [0x240c3e, 0x240c6a) - entry two scratch loads before
 * the push; bounded at the bra-tail to func_022413e6 (0x240c66).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_022413e6(obj, out);                        // random-angle facing tail
 * fidelity: HIGH - matches m2c exactly (func_02240b2e pattern, func_022413e6 tail). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_enter_random_launch(void *obj, s32 *out);      /* 0x022413e6 (tail) */
extern s32  g_060e0f2c[];                             /* s32 flag table, idx obj->0x94 */

void enemy_action_signal_random_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    g_060e0f2c[M32(obj, 0x94)] = 1;
    actor_enter_random_launch(obj, out);
}

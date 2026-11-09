/* enemy_action_signal_then_face [0x02240b2e]: Damage/update, effect-slot signal and facing-response tail. */
#include "dh.h"
/* 0x02240b2e - func_02240b2e: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240b2e, 0x240b5a) - entry is 0x240b2e (scratch-load prologue).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   g_060e0f2c[M32(obj,0x94)] = 1;
 *   func_02241324(obj, out);                 // facing-resolve tail (bra)
 * fidelity: HIGH - exact. index M32(obj,0x94) scales the 4-byte table (shll2). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_enter_directional_launch(void *obj, s32 *out);      /* 0x02241324 (tail) */
extern s32  g_060e0f2c[];                            /* s32 flag table, idx obj->0x94 */

void enemy_action_signal_then_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    g_060e0f2c[M32(obj, 0x94)] = 1;
    actor_enter_directional_launch(obj, out);
}

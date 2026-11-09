#include "dh_work.h"
/* enemy_action_force_direction_face [0x02240c9a]: Damage/update, direction-code facing then response and special flag. */
#include "dh.h"
/* 0x02240c9a - func_02240c9a: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240c9a, 0x240cca) - entry two scratch loads before the push; bounded
 * by the rts at 0x240cc6.
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   func_022417ea(obj);
 *   func_022414ee(obj, out);                        // facing-init (bsr, not tail)
 *   M32(obj,0xE0) |= 0x400000;
 * fidelity: HIGH - matches m2c exactly (func_02240a7a pattern, func_022417ea +
 * func_022414ee). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_face_direction_code(void *obj);                /* 0x022417ea */
extern void actor_enter_lateral_reaction(void *obj, s32 *out);      /* 0x022414ee */

void enemy_action_force_direction_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    actor_face_direction_code(obj);
    actor_enter_lateral_reaction(obj, out);
    ACTOR_M32(obj, state_flags) |= 0x400000;
}

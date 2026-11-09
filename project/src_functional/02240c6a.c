#include "dh_work.h"
/* enemy_action_update_direction_face [0x02240c6a]: Damage/update, optional facing resolution then direction-based response. */
#include "dh.h"
/* 0x02240c6a - func_02240c6a: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240c6a, 0x240c9a) - entry two scratch loads before the push; bounded
 * at the bra-tail to func_022414ee (0x240c96).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   func_022414ee(obj, out);                        // facing-init tail
 * fidelity: HIGH - matches m2c exactly (func_02240a4a pattern, func_022414ee tail). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                  /* 0x00227704 */
extern void actor_enter_lateral_reaction(void *obj, s32 *out);      /* 0x022414ee (tail) */

void enemy_action_update_direction_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    actor_enter_lateral_reaction(obj, out);
}

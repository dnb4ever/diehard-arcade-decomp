#include "dh_work.h"
/* enemy_action_update_then_face [0x02240a4a]: Damage/update and optional facing before shared facing response. */
#include "dh.h"
/* 0x02240a4a - func_02240a4a: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240a4a, 0x240a7a) - entry is 0x240a4a (two scratch loads precede the
 * register pushes; the previous sibling ends with bra @0x240a46).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   func_02241324(obj, out);                 // facing-resolve tail (bra)
 * fidelity: HIGH - exact. 0xE0 read mov.l, bit 0x10000 (reg-form tst). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                  /* 0x00227704 */
extern void actor_enter_directional_launch(void *obj, s32 *out);      /* 0x02241324 (tail) */

void enemy_action_update_then_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    actor_enter_directional_launch(obj, out);
}

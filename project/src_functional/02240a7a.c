#include "dh_work.h"
/* enemy_action_force_face [0x02240a7a]: Direction setup and facing response, then special flag 0x400000. */
#include "dh.h"
/* 0x02240a7a - func_02240a7a: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240a7a, 0x240ae0) - entry is 0x240a7a (scratch-load prologue).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   func_022417a4(obj);
 *   func_02241324(obj, out);                 // bsr (not tail)
 *   M32(obj,0xE0) |= 0x400000;
 * fidelity: HIGH - exact. func_02241324 is a bsr here; the 0xE0 flag OR runs
 * after it, then a plain rts. */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_apply_direction_code(void *obj);                /* 0x022417a4 */
extern void actor_enter_directional_launch(void *obj, s32 *out);      /* 0x02241324 */

void enemy_action_force_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    actor_apply_direction_code(obj);
    actor_enter_directional_launch(obj, out);
    ACTOR_M32(obj, state_flags) |= 0x400000;
}

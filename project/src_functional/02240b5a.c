#include "dh_work.h"
/* enemy_action_update_random_face [0x02240b5a]: Damage/update and optional facing before random-angle response tail. */
#include "dh.h"
/* 0x02240b5a - func_02240b5a: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240b5a, 0x240b8a) - entry is 0x240b5a (scratch-load prologue).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   func_022413e6(obj, out);                 // facing-resolve tail (bra, random-angle variant)
 * fidelity: HIGH - exact. Same shape as func_02240a4a but tails to func_022413e6. */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                  /* 0x00227704 */
extern void actor_enter_random_launch(void *obj, s32 *out);      /* 0x022413e6 (tail) */

void enemy_action_update_random_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    actor_enter_random_launch(obj, out);
}

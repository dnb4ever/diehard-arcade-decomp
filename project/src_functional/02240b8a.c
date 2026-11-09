#include "dh_work.h"
/* enemy_action_force_random_face [0x02240b8a]: Direction setup and random response then special flag 0x400000. */
#include "dh.h"
/* 0x02240b8a - func_02240b8a: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x240b8a, 0x240bba) - entry is 0x240b8a (scratch-load prologue).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   func_022417a4(obj);
 *   func_022413e6(obj, out);                 // bsr (not tail)
 *   M32(obj,0xE0) |= 0x400000;
 * fidelity: HIGH - exact. Same shape as func_02240a7a but calls func_022413e6. */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_apply_direction_code(void *obj);                /* 0x022417a4 */
extern void actor_enter_random_launch(void *obj, s32 *out);      /* 0x022413e6 */

void enemy_action_force_random_face(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    actor_apply_direction_code(obj);
    actor_enter_random_launch(obj, out);
    ACTOR_M32(obj, state_flags) |= 0x400000;
}

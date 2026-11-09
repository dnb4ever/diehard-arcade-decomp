/* enemy_action_init_boosted_motion [0x022407b0]: Damage, optional facing and motion setup; doubles X vector and boosts Y. */
#include "dh_work.h"
#include "dh.h"
/* 0x022407b0 - func_022407b0: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x2407b0, 0x24080e) - bounded at the bra-tail to func_0224124e.
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   func_0224174e(obj);
 *   actor_motion_init(obj);
 *   M32(obj,0xEC) *= 2;  M32(obj,0xF0) += 0x600;
 *   M32(obj,0x108) = 1;  M32(obj,0x84) = 0x1000;
 *   *out = 2;
 *   func_0224124e(obj, out, 3);                    // facing-resolve tail
 * fidelity: HIGH - exact. 0xEC doubled via shll (<<1); 0xF0 += 0x600; out saved
 * in a frame local then forwarded as r5. */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                       /* 0x00227704 */
extern void actor_follow_target_depth(void *obj);                     /* 0x0224174e */
extern void actor_motion_init(void *obj);                 /* 0x022415ee */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */

void enemy_action_init_boosted_motion(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    actor_follow_target_depth(obj);
    actor_motion_init(obj);
    M32(obj, 0xEC) *= 2;
    M32(obj, 0xF0) += 0x600;
    ACTOR_M32(obj, action_phase) = 1;
    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 2;
    actor_select_directional_animation(obj, out, 3);
}

/* enemy_action_enter_statewalk [0x02240fd8]: Damage/update and optional facing, flag 0x40000 and phase=1 then movement tail. */
#include "dh_work.h"
#include "dh.h"
/* 0x02240fd8 - func_02240fd8: per-state "enter statewalk" setup (blob sibling of
 * func_022402c0). real fn [0x240fd8, 0x241024) - bounded at the bra-tail to
 * func_0224124e (0x241020).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   if (M32(obj,0xE0) & 0x10000) func_02227704(obj);
 *   M32(obj,0xE0) |= 0x40000; M32(obj,0x108) = 1; M32(obj,0x84) = 0x1000;
 *   *out = 1;
 *   func_0224124e(obj, out, 0);                      // facing-resolve tail
 * fidelity: HIGH - matches m2c exactly. out saved in a frame local and forwarded
 * as r5; r6 = 0 at the tail. */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_resolve_facing(void *obj);                       /* 0x00227704 */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */

void enemy_action_enter_statewalk(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    if (ACTOR_M32(obj, state_flags) & 0x10000)
        actor_resolve_facing(obj);
    ACTOR_M32(obj, state_flags) |= 0x40000;
    ACTOR_M32(obj, action_phase) = 1;
    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 1;
    actor_select_directional_animation(obj, out, 0);
}

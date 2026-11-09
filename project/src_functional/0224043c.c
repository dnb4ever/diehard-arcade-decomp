/* enemy_action_init_motion [0x0224043c]: Damage/update then motion initialization and shared action continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224043c - func_0224043c: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x24043c, 0x240492) - bounded by the bra-to-tail epilogue at 0x24048e
 * (the census blob [0x2402c0, 0x241556) merges ~30 of these siblings).
 *   actor_apply_damage(obj, M16(obj,0x64));      // per-frame update, r5 = unk64
 *   actor_motion_init(obj);                // actor_motion_init
 *   M32(obj,0x84)  = 0x1000;
 *   M32(obj,0x108) = 1;
 *   if (func_0224158c(obj, 6) == 0) {      // facing/clamp check returned "blocked"
 *       M32(obj,0xBC)  = -1;
 *       M32(obj,0xE0) |= 0x08000000;
 *   }
 *   *out = 2;
 *   func_0224124e(obj, out, 3);            // shared facing-resolve tail (bra)
 * fidelity: HIGH - exact. func_0224158c branch taken when its return == 0
 * (tst r0,r0 / bf skips the block when non-zero). 0xBC/0xE0 stores are mov.l;
 * 0xE0 |= 0x8000000 via base 0xBC + 36. out (r5) is saved then forwarded with
 * r6 = 3 to the tail. No tst #imm8 (reg-form tst only). */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_motion_init(void *obj);                 /* 0x022415ee */
extern s32  actor_debounce_long_event(void *obj, s32 limit);          /* 0x0224158c */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */

void enemy_action_init_motion(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    actor_motion_init(obj);
    ACTOR_M32(obj, action_category) = 0x1000;
    ACTOR_M32(obj, action_phase) = 1;
    if (actor_debounce_long_event(obj, 6) == 0) {
        M32(obj, 0xBC) = -1;
        ACTOR_M32(obj, state_flags) |= 0x08000000;
    }
    *out = 2;
    actor_select_directional_animation(obj, out, 3);
}

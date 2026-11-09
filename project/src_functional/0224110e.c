/* enemy_action_seed_motion_vector [0x0224110e]: Damage/update, phase=1 and motion init then explicit XYZ motion seed. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224110e - func_0224110e: per-state actor setup w/ velocity seed (blob
 * sibling of func_022402c0). real fn [0x24110e, 0x241164) - entry two scratch
 * loads before the push; bounded at the bra-tail to func_0224124e (0x241160).
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   M32(obj,0x108) = 1;
 *   actor_motion_init(obj);
 *   M32(obj,0xEC) = (MU16(obj,0x62) < 0x8000) ? -0x2000 : 0x2000;   // signed seed
 *   M32(obj,0xF4) = -0x2000;
 *   M32(obj,0x84) = 0x1000;
 *   *out = 2;
 *   func_0224124e(obj, out, 3);                     // facing-resolve tail
 * fidelity: HIGH - matches m2c exactly. 0x62 zero-extended, compared >= 0x8000
 * (cmp/ge) to pick the sign of the 0xEC seed. */
extern void actor_apply_damage(void *obj, s32 amount);          /* 0x0222ab18 */
extern void actor_motion_init(void *obj);                 /* 0x022415ee */
extern void actor_select_directional_animation(void *obj, s32 *out, s32 arg);  /* 0x0224124e (tail) */

void enemy_action_seed_motion_vector(void *obj, s32 *out)
{
    actor_apply_damage(obj, M16(obj, 0x64));
    ACTOR_M32(obj, action_phase) = 1;
    actor_motion_init(obj);
    M32(obj, 0xEC) = (ACTOR_MU16(obj, motion_heading) < 0x8000) ? -0x2000 : 0x2000;
    M32(obj, 0xF4) = -0x2000;
    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 2;
    actor_select_directional_animation(obj, out, 3);
}

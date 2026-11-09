/* enemy_action_set_special_or_expire [0x022404ce]: Sets flag 0x400000; height table gate chooses expire or action setup. */
#include "dh_work.h"
#include "dh.h"
/* 0x022404ce - func_022404ce: per-state actor setup (blob sibling of func_022402c0).
 * real fn [0x2404ce, 0x240542) - bounded by the rts epilogue at 0x24053e.
 *   M32(obj,0xE0) |= 0x400000;
 *   if (g_06096c58[(s8)M8(obj,0x34)] > 0x8000) {   // signed s32 table lookup
 *       func_022402c0(obj, out);                     // expire/lock-on check (tail)
 *       return;
 *   }
 *   actor_apply_damage(obj, M16(obj,0x64));
 *   func_022417a4(obj);
 *   base = (M32(obj,0x38) & 0x400) ? 0x0608D898 : 0x06086304;  // s16 table
 *   M16(obj,0x5C) = *(s16 *)(base + (M8(obj,0x4D) & 1) * 2);
 *   M32(obj,0x84) = 0x1000;
 *   *out = 3;
 *   // plain return (no facing-resolve tail)
 * fidelity: HIGH - exact. 0xE0/0x38 read as mov.l; 0x34 is a signed byte scaling
 * the 4-byte g_06096c58 table; the >0x8000 compare is signed (cmp/gt). 0x4D&1
 * selects the 2-byte table entry. No tst #imm8 (reg-form tst only). */
extern void actor_apply_damage(void *obj, s32 amount);     /* 0x0222ab18 */
extern void actor_apply_direction_code(void *obj);                /* 0x022417a4 */
extern void actor_check_expiry_or_lock(void *obj, s32 *out);       /* 0x022402c0 (tail) */
extern s32  g_06096c58[];                            /* s32 table, idx (s8)obj->0x34 */

void enemy_action_set_special_or_expire(void *obj, s32 *out)
{
    ACTOR_M32(obj, state_flags) |= 0x400000;
    if (g_06096c58[ACTOR_M8(obj, slot_id)] > 0x8000) {
        actor_check_expiry_or_lock(obj, out);
        return;
    }
    actor_apply_damage(obj, M16(obj, 0x64));
    actor_apply_direction_code(obj);
    {
        s16 *base = (s16 *)(uintptr_t)((ACTOR_M32(obj, render_flags) & 0x400) ? 0x0608D898 : 0x06086304);
        ACTOR_M16(obj, animation_requested) = base[ACTOR_M8(obj, direction_code) & 1];
    }
    ACTOR_M32(obj, action_category) = 0x1000;
    *out = 3;
}

/* enemy_route_category_a_response [0x0223e690]: Routes the first category's response by combat flags and descriptor dispatch results. */
#include "dh.h"

/* 0x0223e690 - flag-gated command router (AI/behaviour dispatch), "cmd_router_a".
 *
 * MIRAGE CORRECTION: fn.py reports code 0x78a / extent 0x848 and the prior
 * bandbig23 pass measured a 1930-byte, 36-branch "monster".  That is the whole
 * literal-pool translation unit [0x23e5e0,0x23eeb4) - a run of ~10 sibling
 * functions sharing one pool.  The REAL func_0223e690 is only
 * [0x0223e690, 0x0223e878)  (~244 insn, 13 computed branches); it ends at the
 * rts/epilogue at 0x23e874.  Everything from 0x23e878 on (fresh mov.l r14,@-r15
 * prologues) is independent siblings (func_0223e878, func_0223e8ca,
 * func_0223e96c, func_0223e9d2, func_0223eb3c, func_0223ec44, func_0223ed14,
 * func_0223ed94, func_0223edce) that fn.py's extent swallowed.  Same failure
 * mode documented for the "giants" in notes/functional_lastgiants.md.
 *
 * Shape: entA = ent->0x158; tbl = entA->4 (halfword lookup base, may be NULL);
 * flags = ent->0xE0.  A chain of guarded tail-calls (restore-then-braf) and
 * predicates, two inner switch()es on func_02244e2e and func_02243fe0.
 *
 * All 13 branches resolve (braf_resolve.py).  The "bsrf r0 index table" at
 * 0x23e72c is the PHANTOM mask literal 0x0003FFFF (tst r3,r2 on ent->0x8C),
 * NOT a jump table.
 *
 * m2c notes: m2c cannot scaffold this TU (chokes on a pool datum,
 * "Failed to parse: stc tbr,r1") - 100% hand-traced against fn.py.
 *   - The stack slot written at 0x23e7aa ( **(ent->0x174) ) is NOT an argument
 *     to func_02243fe0 (which is 1-arg, see shipped 02243fe0.c and the sibling
 *     func_0223f29a which calls it with no stack setup).  It is a hoisted local
 *     ('saved') consumed much later at 0x23e860 as the 2nd arg of the tail call
 *     func_022473ee(ent, saved).
 *   - tst #2 on ent->0x54 is a true 16-bit mask (bit 1), not sign-extended.
 * fidelity: HIGH. */

s32   enemy_enter_guard(void *ent);                 /* 0x02247d6e */
void  enemy_reset_idle_or_walk(void *ent);                 /* 0x022469f2 */
void *enemy_find_latched_effect_target(void *ent);                 /* 0x02245d1c */
void  enemy_begin_combo(void *ent, void *a1);        /* 0x02249836 */
s32   actor_state_dispatch_entry(void *ent);                 /* 0x02244e2e */
void  enemy_enter_descriptor_state18(void *ent);                 /* 0x022470a2 */
void  enemy_dispatch_script(void *ent);                 /* 0x02247036 */
s32   enemy_sweep_toward_target(void *ent);                 /* 0x02243fe0 (shipped) */
void  actor_set_state_7_entry(void *ent);                 /* 0x0224840a */
void  enemy_enter_state19(void *ent);                 /* 0x02249fb0 */
void  enemy_begin_scripted_dash(void *ent, s32 saved);       /* 0x022473ee */

void enemy_route_category_a_response(void *ent)
{
    void *entA  = MP(ent, 0x158);
    void *tbl   = MP(entA, 4);              /* r13 - halfword table base */
    s32   flags = M32(ent, 0xE0);

    if (flags & 0x100) {
        if (enemy_enter_guard(ent)) {
            return;
        }
        enemy_reset_idle_or_walk(ent);                /* tail */
        return;
    }

    if ((M32(ent, 0x8C) & 0x3FFFF) == 0 && (flags & 0x40)) {
        void *r5 = enemy_find_latched_effect_target(ent);
        if (r5 != 0) {
            enemy_begin_combo(ent, r5);        /* tail */
            return;
        }
    }

    switch (actor_state_dispatch_entry(ent)) {
    case 1:
        enemy_dispatch_script(ent);                /* tail */
        return;
    case 3:
        enemy_enter_descriptor_state18(ent);                /* tail */
        return;
    case 2:
        enemy_enter_descriptor_state18(ent);                /* call, then common path */
        break;
    default:                               /* 0 and anything else */
        break;
    }

    if (!(MU16(ent, 0x54) & 2)) {
        if (tbl == 0) {
            return;
        }
        if (M8(ent, 0x40) != 0) {
            return;
        }
        if (MU16(tbl, (M32(ent, 0x100) << 1)) != MU16(ent, 0x58)) {
            return;
        }
    }

    /* 0x23e7a2 - hoisted: value consumed only by the func_022473ee tail call */
    {
        s32 saved = M32(MP(MP(ent, 0x174), 0), 0);

        switch (enemy_sweep_toward_target(ent)) {
        case 1:
            actor_set_state_7_entry(ent);
            M16(ent, 0x5C) = M16(ent, 0x14A);
            return;
        case 2:
            enemy_enter_state19(ent);            /* tail */
            return;
        default:                           /* 0x23e804 */
            break;
        }

        {
            s32 idx128 = M32(ent, 0x128);
            u32 sel = *(u32 *)(uintptr_t)(0x060995D0
                        + ((u32)(s32)M8(ent, 0x34) << 5)
                        + ((u32)idx128 << 2));
            s32 flags2 = M32(ent, 0xE0);    /* re-read: callees may mutate 0xE0 */
            s32 pub;
            u16 pub16;

            if (sel < 0x40000u || sel >= 0x190000u) {
                return;
            }
            if (!(flags2 & 0x10)) {
                return;
            }
            if (flags2 & 0x200) {
                return;
            }

            pub = M32(0x0609740C, 0);
            if (M8(ent, 0x44) != 0) {
                pub += 8;
            }
            pub16 = (u16)pub;
            if ((s32)pub16 < 10 || (s32)pub16 > 14) {
                return;
            }

            M32(ent, 0x120) = idx128;
            enemy_begin_scripted_dash(ent, saved);     /* tail */
        }
    }
}

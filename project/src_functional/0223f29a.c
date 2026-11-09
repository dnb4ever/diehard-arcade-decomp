/* enemy_route_category_b_response [0x0223f29a]: Routes the second category's response by combat flags and state results. */
#include "dh.h"

/* 0x0223f29a - the real large flag-gated command router of the f10e family
 *   (the "cmd_router_b" the brief/bandbig23 was describing; its address was
 *   mis-attributed to the small handler at 0x23f10e by the pool-unit mirage).
 *
 * Real extent [0x0223f29a, 0x0223f494) (~127 insn), ending at the rts at
 * 0x23f490.  Reserves no stack local.  state = ent->0xD8 selects:
 *   state==0 -> the e690-mirror predicate chain (see 0223e690.c; identical
 *               guards and both inner switch()es; confirmed by cross-check).
 *   state==1 -> inline "boss/trigger" construction.
 *   else     -> return.
 *
 * All branches resolve.  Phantom table @0x23f3dc = mask 0x0003FFFF
 * (tst r3,r2 on ent->0x8C), NOT a jump table.  func_02243fe0 is called with no
 * stack setup here, confirming it is 1-arg (settles the 0223e690.c sp[0] note).
 *
 * m2c: all-M2C_ERROR; hand-traced.  tst #2 on ent->0x54 is a true 16-bit mask.
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
void  enemy_begin_approach(void *ent);                 /* 0x02246cac */
s32   enemy_arm_distance_band(void *ent);                 /* 0x0223f494 (sibling) */

void enemy_route_category_b_response(void *ent)
{
    void *entA  = MP(ent, 0x158);
    void *tbl   = MP(entA, 4);              /* r13 - halfword table base */
    s32   state = M32(ent, 0xD8);

    if (state == 1) {
        /* 0x23f424 - inline construction */
        if (MU16(ent, 0x54) & 2) {
            enemy_begin_approach(ent);            /* tail */
            return;
        }
        if (MU16(ent, 0x58) != 10) {
            return;
        }
        if (M8(ent, 0x40) != 0) {
            return;
        }
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= M32(ent, 0x100);
        if (M32(DH_PLAY_SESSION_MODE_ADDR, 0) != 1) {
            return;
        }
        if (M32(DH_VIEW_REQUEST_PRIORITY_ADDR, 0) >= 1) {
            void *p = MP(DH_VIEW_TASK_ADDR, 0);
            void *q = MP(p, 8);
            M32(DH_VIEW_REQUEST_PRIORITY_ADDR, 0) = 1;
            M8(p, 13) = 1;
            M16(q, 0xAE) = 0;
            M16(q, 0xB0) = 0;
            M16(q, 0xB2) = 0;
        }
        return;
    }

    if (state != 0) {
        return;
    }

    /* 0x23f300 - predicate chain (mirrors func_0223e690) */
    {
        s32 flags = M32(ent, 0xE0);

        if (flags & 0x100) {
            if (enemy_enter_guard(ent)) {
                return;
            }
            enemy_reset_idle_or_walk(ent);            /* tail */
            return;
        }

        if ((M32(ent, 0x8C) & 0x3FFFF) == 0 && (flags & 0x40)) {
            void *r5 = enemy_find_latched_effect_target(ent);
            if (r5 != 0) {
                enemy_begin_combo(ent, r5);    /* tail */
                return;
            }
        }

        switch (actor_state_dispatch_entry(ent)) {
        case 1:
            enemy_dispatch_script(ent);            /* tail */
            return;
        case 3:
            enemy_enter_descriptor_state18(ent);            /* tail */
            return;
        case 2:
            enemy_enter_descriptor_state18(ent);
            break;
        default:
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

        switch (enemy_sweep_toward_target(ent)) {
        case 1:
            actor_set_state_7_entry(ent);
            M16(ent, 0x5C) = M16(ent, 0x14A);
            return;
        case 2:
            enemy_enter_state19(ent);            /* tail */
            return;
        default:                           /* 0x23f418 */
            enemy_arm_distance_band(ent);
            return;
        }
    }
}

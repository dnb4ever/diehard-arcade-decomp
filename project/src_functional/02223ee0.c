#include "dh_work.h"
/* player_update_combat_input [0x02223ee0]: Reads player input/edge records and dispatches action-category and combat state. */
#include "dh.h"

/* 0x02223ee0 - func_02223ee0: per-player input / combat state machine.
 * (symbols.txt lists this address as actor_behavior_fsm - same function.)
 *
 * void func_02223ee0(void *entity).  Single function, single epilogue @0x225fa2;
 * extent [0x02223ee0, 0x02225fac) code + trailing pool to 0x225fe8 (~4150 insn).
 *
 * Shape: resolve the player slot (M8(e,0x34)) and its two input records
 * (g_06098734 stride 0x18, g_060987AC stride 6); decode the three button words;
 * then run a pipeline of category dispatchers keyed on the action-category
 * bitfield cat=M32(e,0x84) and the state st=M32(e,0x88).  cat/st are working
 * copies committed back to +0x84/+0x88 at the epilogue.
 *
 * The ONLY real jump table is @0x22403e: switch((st)-14) for states 14..23,
 * reached when (cat & 0x180).  Table @0x224044 (s16, target = 0x224042 + entry):
 *   st14->0x224058 st15->0x2240ce st16->0x2241c2 st17->0x2242c2 st18->0x224340
 *   st19->0x22436c st20->0x224384 st21->0x22439c st22->0x2242f4 st23->0x22430e
 *   default->0x2243ca (no-op).  Decoded by hand from ROM bytes + pool literals.
 *
 * Early combo-replay path: if the per-slot replay buffer is non-empty
 * (MU16(g_06096E90,slot*2)!=0) the braf r1 @0x223fb8 jumps straight to 0x225c12
 * (goto L_combo), skipping all input dispatch - the player is replaying a queued
 * combo.  func_02225fac then emits the buffered interaction records.
 *
 * Computed branches resolved: braf r1@0x223fb8->0x225c12 (goto); the 10-way
 * table@0x22403e; bsrf@0x223fc2->func_0220fb98; bsrf x4->func_0223c7f6.
 *
 * CORRECTIONS vs m2c (tools/decomp/m2c_fn.py):
 *  - m2c sign-extends `tst #imm8` immediates >=0x80: it renders `tst #192` as
 *    `& ~0x3F` (0xFFC0) and `tst #128` as `& ~0x7F` (0xFF80).  The hardware
 *    zero-extends, so the real masks are `& 0xC0` and `& 0x80` (low byte only).
 *    All such sites are corrected here (button remap, dir-accel, edge detect).
 *  - the four bsrf r3 are func_0223c7f6(entity); m2c modelled the undefined return
 *    with a stale local - the conditionals are rewritten on the real return.
 *  - the "bsr" at 0x22585a/0x225b52/0x224016 are pool phantoms, not calls; the
 *    only direct call is bsr@0x225c6c -> func_02225fac.  0x225d9c braf is a pool
 *    phantom too (0x0023 word in a literal pool).
 *
 * fidelity: HIGH.  Preamble, the full jump table, all 5 computed transfers, the
 * combo-replay tail and the epilogue were hand-traced against fn.py disassembly.
 * The intermediate category dispatchers are the m2c reconstruction (faithful on
 * every block spot-checked) with the mask fixes above applied.
 */

extern void input_update_direction_tap(void *rec, void *rec2);          /* bsrf@0x223fc2 */
extern s32  actor_step_effect_transition(void *e);                        /* bsrf x4, returns tested flag */
extern void interaction_append_slot_records(void *e, void *src_desc, void *arr_base, s16 flags);

void player_update_combat_input(void *arg0)
{
    void *e = arg0;
    s32  slot = ACTOR_WORK(e)->slot_id;                              /* sp8: player slot (1-based) */
    void *rec  = (void *)(uintptr_t)(DH_PLAYER_INPUT_RECORDS_ADDR_U + (s8)((slot - 1) * 0x18)); /* sp28: g_06098734 record */
    void *rec2 = (void *)(uintptr_t)(DH_PLAYER_DIRECTION_TAPS_ADDR_U + (s8)((slot - 1) * 6));    /* sp24: g_060987AC record */
    s16  b32 = PLAYER_INPUT(rec)->buttons;                               /* sp32: current buttons */
    s16  b30 = PLAYER_INPUT(rec)->pressed;                               /* sp30 */
    s16  b2e = PLAYER_INPUT(rec)->released;                               /* sp2e */
    s16  b2c;                                             /* sp2c: scratch mask */
    s32  cat = ACTOR_WORK(e)->action_category;                              /* sp4: action-category bits */
    s32  st  = ACTOR_WORK(e)->action_state;                              /* sp0: state */
    s32  i;
    void *arr_base, *src_desc;

    /* --- button remap when facing flipped (unk44) --- */
    if (ACTOR_WORK(e)->facing_right != 0) {
        if ((u16)b32 & DH_PAD_HORIZONTAL) b32 = (s16)(b32 ^ DH_PAD_HORIZONTAL);
        if ((u16)b32 & DH_PAD_DEPTH)  b32 = (s16)(b32 ^ DH_PAD_DEPTH);   /* tst #192 -> 0xC0 */
        if ((u16)b30 & DH_PAD_HORIZONTAL) b30 = (s16)(b30 ^ DH_PAD_HORIZONTAL);
        if ((u16)b30 & DH_PAD_DEPTH)  b30 = (s16)(b30 ^ DH_PAD_DEPTH);
    }

    /* --- early combo-replay exit (braf r1 @0x223fb8) --- */
    if (MU16((void *)0x06096E90u, slot * 2) != 0)
        goto L_combo;

    input_update_direction_tap(rec, rec2);                              /* bsrf @0x223fc2 */

    /* ===== dispatch-1: cat 0x200 / 0x40+0xC / 0x180-jumptable ===== */
    if (cat & 0x200) {
        if (ACTOR_WORK(e)->animation_events & 2) {
            ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
            cat = 1;
            ACTOR_M16(e, interaction_latch) = 0;
        }
    } else if (!(cat & 0x180)) {
        if (!(cat & 0x40)) {
            if ((cat & 0xC) && !((u16)b32 & DH_PAD_DIRECTIONS)) {
                if (((u32)(M32((void *)DH_FRAME_TICK_ADDR_U, 0) - ACTOR_M32(e, action_start_tick)) <= 3U) &&
                    (M8(rec2, 5) != 0) &&
                    (M8((void *)0x06087974u, M8(e, 0x45) * 4) != 0)) {
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E38u, M8(e, 0x45) * 2);
                    cat = 0x40;
                    st  = 9;
                    ACTOR_M16(e, interaction_latch) = 1;
                    PLAYER_INPUT(rec)->direction_hold_frames = 0;
                } else {
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    cat = 1;
                    st  = 0;
                    ACTOR_M16(e, interaction_latch) = 0;
                }
            }
        } else {
            switch (st) {                                /* cat & 0x40 (0x2243e6) */
            case 11:
                if ((s32)ACTOR_U16(e, animation_frame) >= 7) {
                    ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                    st = 0xC;
                }
                break;
            case 12:
                if (ACTOR_WORK(e)->animation_events & 2) {
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    cat = 1;
                    st  = 0;
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                break;
            case 9:
                st += 1;
                break;
            case 10:
                if ((s32)ACTOR_U16(e, animation_frame) < 6) {
                    if ((u16)b30 & DH_PAD_ACTION1) {
                        st = 0xB;
                        ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087E68u + (slot - 1) * 2),
                                           M8(e, 0x45) * 4);
                    } else if ((u16)b30 & DH_PAD_ACTION2) {
                        st = 0xB;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E98u, M8(e, 0x45) * 2);
                    }
                } else if ((PLAYER_INPUT(rec)->direction_hold_frames > 9) && ((u16)b32 & DH_PAD_X_NEG)) {
                    cat = 0x80;
                    st  = 0xE;
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_WORK(e)->animation_requested = M16(MP((void *)0x06087D48u, M8(e, 0x45) * 4), 0);
                } else if (ACTOR_WORK(e)->animation_events & 2) {
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    cat = 1;
                    st  = 0;
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                break;
            case 13:
                if ((M8((void *)0x06087975u, M8(e, 0x45) * 4) != 0) &&
                    (PLAYER_INPUT(rec)->direction_hold_frames > 8) && ((u16)b32 & DH_PAD_X_POS)) {
                    cat = 0x200;
                    st  = 0;
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_WORK(e)->animation_requested = 0x3D;
                } else if (ACTOR_WORK(e)->animation_events & 2) {
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    cat = 1;
                    st  = 0;
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                break;
            }
        }
    } else {
        /* ===== MAIN jump table @0x22403e: states 14..23 ===== */
        switch (st) {
        case 14:                                         /* 0x224058 */
            if (ACTOR_WORK(e)->animation_events & 1) {
                ACTOR_WORK(e)->animation_requested = M16(MP((void *)0x06087D48u, M8(e, 0x45) * 4), 2);
                st = 15;
            }
            if ((u16)b30 & DH_PAD_ACTION2) {
                M32(e, 0xC8) = M16((void *)0x06087DA8u, M8(e, 0x45) * 2);
                cat = 0x100;
                st  = 22;
            } else if ((u16)b30 & DH_PAD_ACTION1) {
                M32(e, 0xC8) = M16((void *)0x06087D90u, M8(e, 0x45) * 2);
                cat = 0x100;
                st  = 18;
            }
            break;

        case 15:                                         /* 0x2240ce */
            if ((u16)b30 & DH_PAD_ACTION2) {
                M32(e, 0xC8) = ((u16)M16(e, 0x88) > 13)
                             ? M16((void *)0x06087DC0u, M8(e, 0x45) * 2)
                             : M16((void *)0x06087DA8u, M8(e, 0x45) * 2);
                cat = 0x100;
                st  = 22;
            } else if ((u16)b30 & DH_PAD_ACTION1) {
                M32(e, 0xC8) = ((u16)M16(e, 0x88) > 13)
                             ? M16((void *)0x06087D90u, M8(e, 0x45) * 2)
                             : M16((void *)0x06087D78u, M8(e, 0x45) * 2);
                cat = 0x100;
                st  = 18;
            } else if ((u16)b32 & DH_PAD_X_NEG) {
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(e)->animation_requested = M16(MP((void *)0x06087D48u, M8(e, 0x45) * 4), 4);
                st = 17;
            } else if (ACTOR_WORK(e)->animation_events & 1) {
                st = 16;
            }
            break;

        case 16:                                         /* 0x2241c2 */
            if ((u16)b30 & DH_PAD_ACTION1) {
                M32(e, 0xC8) = ((u16)M16(e, 0x88) > 14 || (u16)M16(e, 0x88) < 5)
                             ? M16((void *)0x06087D90u, M8(e, 0x45) * 2)
                             : M16((void *)0x06087D78u, M8(e, 0x45) * 2);
                cat = 0x100;
                st  = 18;
            } else if ((u16)b30 & DH_PAD_ACTION2) {
                M32(e, 0xC8) = ((u16)M16(e, 0x88) > 16 || (u16)M16(e, 0x88) < 6)
                             ? M16((void *)0x06087DC0u, M8(e, 0x45) * 2)
                             : M16((void *)0x06087DA8u, M8(e, 0x45) * 2);
                cat = 0x100;
                st  = 22;
            } else if (!((u16)b32 & DH_PAD_X_NEG)) {
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(e)->animation_requested = M16(MP((void *)0x06087D48u, M8(e, 0x45) * 4), 4);
                st = 17;
            }
            break;

        case 17:                                         /* 0x2242c2 */
            if (ACTOR_WORK(e)->animation_events & 2) {
                ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                cat = 1;
                st  = 0;
                ACTOR_M16(e, interaction_latch) = 0;
            }
            break;

        case 18:                                         /* 0x224340 */
            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(e)->animation_requested = M16(e, 0xCA);
            st = (M8(e, 0x45) == 0) ? 19 : 21;
            break;

        case 19:                                         /* 0x22436c */
            if ((u16)M16(e, 0x88) == 9)
                st = 20;
            break;

        case 20:                                         /* 0x224384 */
            if ((u16)M16(e, 0x88) == 16)
                st = 21;
            break;

        case 21:                                         /* 0x22439c */
            if (ACTOR_WORK(e)->animation_events & 2) {
                ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                cat = 1;
                st  = 0;
                ACTOR_M16(e, interaction_latch) = 0;
            }
            break;

        case 22:                                         /* 0x2242f4 */
            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(e)->animation_requested = M16(e, 0xCA);
            st = 23;
            break;

        case 23:                                         /* 0x22430e */
            if (ACTOR_WORK(e)->animation_events & 2) {
                ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                cat = 1;
                st  = 0;
                ACTOR_M16(e, interaction_latch) = 0;
            }
            break;

        default:                                         /* 0x2243ca */
            break;
        }
    }

    /* ===== dispatch-2: cat 0x10 / 0x20 / 0x100000 / 0x200000 / 0x410 ===== */
    if (!(cat & 0xD)) {
        if (!(cat & 0x10)) {
            if (cat & 0x20) {
                switch (st) {
                case 3:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        ACTOR_WORK(e)->facing_right  = (s8)(ACTOR_WORK(e)->facing_right ^ 1);
                        ACTOR_WORK(e)->angle_y = (s16)(ACTOR_WORK(e)->angle_y + 0x8000);
                        st = 4;
                        ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    }
                    break;
                case 4:
                    if (actor_step_effect_transition(e) == 0) {
                        cat = 1;
                        st  = 0;
                        ACTOR_M16(e, interaction_latch) = 0;
                    } else {
                        cat = ACTOR_WORK(e)->action_category;
                    }
                    break;
                }
            } else if (!(cat & 0x100000)) {
                if (cat & 0x200000) {
                    s32 s5 = ACTOR_WORK(e)->action_state;
                    switch (s5) {
                    case 0x34:
                    case 0x38:
                        M32(e, 0xC8) = (s32)(M32(e, 0xC8) + M16(e, 0xCE));
                        M32(e, 0x7C) = (s32)(-(M32(e, 0xC8) * M32(e, 0xC8)) + 0x9000);
                        if (M16(e, 0xCC) > 4) {
                            M16(e, 0xCE) = (s16)(M16(e, 0xCE) - 1);
                        } else if (M32(e, 0x78) != 0) {
                            M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0)
                                         ? (s32)(M32(e, 0x78) + 0x1A0)
                                         : (s32)(M32(e, 0x78) - 0x1A0);
                        }
                        if (M32(e, 0x7C) <= 0) {
                            ACTOR_WORK(e)->animation_requested = (s5 == 0x34)
                                ? M16((void *)(uintptr_t)(0x06087C54u + M8(e, 0x45) * 4), M32(e, 0x98) * 2)
                                : M16((void *)(uintptr_t)(0x06087CD8u + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
                            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                            cat = 0x100000;
                            st  = 0x39;
                            M32(e, 0xC4) = 1;
                            PLAYER_INPUT(rec)->direction_hold_frames = 0;
                            M32(e, 0x7C) = 0;
                            M32(e, 0x78) = 0;
                            M32(e, 0x98) = 0;
                        }
                        break;
                    case 0x33:
                    case 0x37:
                        M16(e, 0xCE) = (s16)(M16(e, 0xCE) - 1);
                        M32(e, 0xC8) = (s32)(M32(e, 0xC8) + M16(e, 0xCE));
                        M32(e, 0x7C) = (s32)(-(M32(e, 0xC8) * M32(e, 0xC8)) + 0x12100);
                        if (M32(e, 0x78) != 0) {
                            M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0)
                                         ? (s32)(M32(e, 0x78) + 0x1A0)
                                         : (s32)(M32(e, 0x78) - 0x1A0);
                        }
                        if (M32(e, 0x7C) <= 0) {
                            ACTOR_WORK(e)->animation_requested = (s5 == 0x33)
                                ? M16((void *)(uintptr_t)(0x06087C54u + M8(e, 0x45) * 4), M32(e, 0x98) * 2)
                                : M16((void *)(uintptr_t)(0x06087CD8u + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
                            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                            cat = 0x100000;
                            st  = 0x39;
                            M32(e, 0xC4) = 1;
                            PLAYER_INPUT(rec)->direction_hold_frames = 0;
                            M32(e, 0x7C) = 0;
                            M32(e, 0x78) = 0;
                            M32(e, 0x98) = 0;
                        }
                        break;
                    }
                }
            } else {
                switch (st) {
                case 0x39:
                    if (M32(e, 0xC4) != 0) {
                        M32(e, 0xC4) = (s32)(M32(e, 0xC4) - 1);
                        if ((M8((void *)0x06087977u, M8(e, 0x45) * 4) != 0) && ((u16)b30 & DH_PAD_ACTION1)) {
                            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                            ACTOR_WORK(e)->animation_requested = M16((void *)0x06087D04u, M8(e, 0x45) * 2);
                            st = 0x3A;
                        } else if ((M8((void *)0x06087977u, M8(e, 0x45) * 4) != 0) && ((u16)b30 & DH_PAD_ACTION2)) {
                            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                            ACTOR_WORK(e)->animation_requested = M16((void *)0x06087D1Au, M8(e, 0x45) * 2);
                            st = 0x3A;
                        }
                    }
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                        cat = 1;
                        st  = 0;
                        ACTOR_M16(e, interaction_latch) = 0;
                    }
                    break;
                case 0x3A:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                        cat = 1;
                        st  = 0;
                        ACTOR_M16(e, interaction_latch) = 0;
                    }
                    break;
                case 0x31:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        cat = 0x200000;
                        st  = 0x33;
                        ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087C28u + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
                    }
                    break;
                case 0x35:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        cat = 0x200000;
                        st  = 0x37;
                        ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087CACu + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
                    }
                    break;
                case 0x32:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        cat = 0x200000;
                        st  = 0x34;
                        ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087C28u + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
                    }
                    break;
                case 0x36:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        cat = 0x200000;
                        st  = 0x38;
                        ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087CACu + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
                    }
                    break;
                }
            }
        } else {
            switch (st) {                                /* cat & 0x10 */
            case 4:
                b2c = (s16)(b32 & 0x1C0);
                if ((u16)b2c != 0) {
                    ACTOR_WORK(e)->animation_requested = M16(MP((void *)0x06087A64u, M8(e, 0x45) * 4),
                                       ((u16)b2c >> 6) * 2);
                    ACTOR_M16(e, interaction_latch) = 0;
                    cat = 4;
                    st  = ((u16)b2c & DH_PAD_X_NEG) ? 1 : 2;
                } else {
                    cat = 1;
                    st  = 0;
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                break;
            case 5:
                if (ACTOR_WORK(e)->animation_events & 2) {
                    cat = 1;
                    st  = 0;
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                break;
            case 3:
                if (ACTOR_WORK(e)->animation_events & 2) {
                    ACTOR_WORK(e)->facing_right  = (s8)(ACTOR_WORK(e)->facing_right ^ 1);
                    ACTOR_WORK(e)->angle_y = (s16)(ACTOR_WORK(e)->angle_y + 0x8000);
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087DDAu, M8(e, 0x45) * 4);
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    cat = 4;
                    st  = 4;
                    break;
                }
                if (((s32)ACTOR_U16(e, animation_frame) < 2) && ((u16)b32 & DH_PAD_X_POS)) {
                    if ((u16)b30 & DH_PAD_ACTION1) {
                        cat = 0x20;
                        st  = 3;
                        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E08u, M8(e, 0x45) * 2);
                    } else if ((u16)b30 & DH_PAD_ACTION2) {
                        cat = 0x20;
                        st  = 3;
                        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E20u, M8(e, 0x45) * 2);
                    }
                }
                if (((s32)ACTOR_U16(e, animation_frame) >= 2) || (M8(rec2, 5) == 0) ||
                    (M8((void *)0x06087974u, M8(e, 0x45) * 4) == 0)) {
                    /* no transition */
                } else {
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E50u, M8(e, 0x45) * 2);
                    cat = 0x40;
                    st  = 0xD;
                    ACTOR_M16(e, interaction_latch) = 1;
                    PLAYER_INPUT(rec)->direction_hold_frames = 0;
                }
                break;
            }
        }
    } else if (((u16)b2e & DH_PAD_ACTION1) &&
               (M16((void *)0x0609979Cu, (slot - 1) * 2) > 0x16) &&
               (M8(e, 0x45) == 0)) {
        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_WORK(e)->animation_requested = 0x82;
        cat = 0x410;
        st  = 5;
        ACTOR_M16(e, interaction_latch) = 1;
    } else if (((u16)b2e & DH_PAD_ACTION2) &&
               (M16((void *)0x060997A0u, (slot - 1) * 2) > 0x16) &&
               ((M8(e, 0x45) <= 2) || (M32((void *)DH_ACTOR_SLOT_MASKS_ADDR_U, M8(e, 0x45) * 4) & 0x3E0))) {
        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_WORK(e)->animation_requested = 0x84;
        cat = 0x410;
        st  = 5;
        ACTOR_M16(e, interaction_latch) = 1;
    } else {
        b2c = (s16)(b32 & DH_PAD_DIRECTIONS);
        if ((u16)b2c == 0) {
            /* none */
        } else if ((u16)b2c & DH_PAD_X_POS) {
            ACTOR_M16(e, interaction_latch)  = 1;
            ACTOR_M32(e, action_start_tick) = (s32)M32((void *)DH_FRAME_TICK_ADDR_U, 0);
            if ((u16)b30 & DH_PAD_ACTION1) {
                cat = 0x20;
                st  = 3;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E08u, M8(e, 0x45) * 2);
            } else if ((u16)b30 & DH_PAD_ACTION2) {
                cat = 0x20;
                st  = 3;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087E20u, M8(e, 0x45) * 2);
            } else {
                cat = 0x10;
                st  = 3;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087DD8u, M8(e, 0x45) * 4);
            }
        } else if ((u16)ACTOR_WORK(e)->animation_requested ==
                   (u16)M16(MP((void *)0x06087A64u, M8(e, 0x45) * 4), ((u16)b2c >> 6) * 2)) {
            /* already in that pose */
        } else {
            ACTOR_WORK(e)->animation_requested = M16(MP((void *)0x06087A64u, M8(e, 0x45) * 4), ((u16)b2c >> 6) * 2);
            ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
            if ((u16)b2c & DH_PAD_X_NEG) {
                st = 1;
                if (cat & 1)
                    ACTOR_M32(e, action_start_tick) = (s32)M32((void *)DH_FRAME_TICK_ADDR_U, 0);
            } else {
                st = 2;
            }
            cat = 4;
            ACTOR_M16(e, interaction_latch) = 0;
        }
    }

    /* ===== dispatch-3: cat 0x8000 (switch 7) / else physics + switch 6 ===== */
    if (!(cat & 0x10000)) {
        if (cat & 0x8000) {
            switch (st) {
            case 24:
                if (ACTOR_WORK(e)->animation_events & 1)
                    st = 0x19;
                break;
            case 25:
                if (ACTOR_WORK(e)->animation_events & 1) {
                    cat = 0x10000;
                    st  = 0x1A;
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A96u, M8(e, 0x45) * 8);
                }
                break;
            case 36:
                if (ACTOR_WORK(e)->animation_events & 2) {
                    if (actor_step_effect_transition(e) == 0) {
                        ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                        cat = 1;
                        st  = 0;
                    } else {
                        cat = ACTOR_WORK(e)->action_category;
                    }
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                break;
            case 37:
                if (ACTOR_WORK(e)->animation_events & 2) {
                    ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                    cat = 1;
                    st  = 0;
                    ACTOR_M16(e, interaction_latch) = 0;
                }
                M32(e, 0x80) = 0;
                M32(e, 0x7C) = 0;
                M32(e, 0x78) = 0;
                ACTOR_WORK(e)->position_y  = 0;
                ACTOR_WORK(e)->movement_y = 0;
                ACTOR_WORK(e)->movement_x = 0;
                break;
            case 34:
                if (M32(e, 0xC4) != 0) {
                    M32(e, 0xC4) = (s32)(M32(e, 0xC4) - 1);
                    if (((u16)b30 & DH_PAD_ACTION1) && (M8((void *)0x06087977u, M8(e, 0x45) * 4) != 0)) {
                        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087D04u, M8(e, 0x45) * 2);
                        st = 0x24;
                    } else if (((u16)b30 & DH_PAD_ACTION2) && (M8((void *)0x06087977u, M8(e, 0x45) * 4) != 0)) {
                        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087D1Au, M8(e, 0x45) * 2);
                        st = 0x24;
                    } else {
                        goto b376;
                    }
                } else {
                b376:
                    if (ACTOR_WORK(e)->animation_events & 2) {
                        if (actor_step_effect_transition(e) == 0) {
                            ACTOR_WORK(e)->animation_requested = (s16)M32((void *)0x06096C74u, slot * 4);
                            cat = 1;
                            st  = 0;
                        } else {
                            cat = ACTOR_WORK(e)->action_category;
                        }
                        ACTOR_M16(e, interaction_latch) = 0;
                        M32(e, 0x80) = 0;
                        M32(e, 0x7C) = 0;
                        M32(e, 0x78) = 0;
                        ACTOR_WORK(e)->position_y  = 0;
                        ACTOR_WORK(e)->movement_y = 0;
                        ACTOR_WORK(e)->movement_x = 0;
                    }
                }
                break;
            }
        }
    } else {
        /* cat & 0x10000: horizontal-drift physics + switch 6 */
        M32(e, 0xC8) = (s32)(M32(e, 0xC8) + M16(e, 0xCE));
        M32(e, 0x7C) = (s32)(-(M32(e, 0xC8) * M32(e, 0xC8)) + 0x16900);
        if ((u16)b32 & DH_PAD_X_NEG) {
            s32 mag = (M32(e, 0x78) >= 0) ? M32(e, 0x78) : -M32(e, 0x78);
            if (mag < 0x1500)
                M32(e, 0x78) = (s32)(M32(e, 0x78) + ((ACTOR_WORK(e)->facing_right != 0) ? -0xE0 : 0xE0));
        } else if ((u16)b32 & DH_PAD_X_POS) {
            s32 mag = (M32(e, 0x78) >= 0) ? M32(e, 0x78) : -M32(e, 0x78);
            if (mag < 0x1500)
                M32(e, 0x78) = (s32)(M32(e, 0x78) + ((ACTOR_WORK(e)->facing_right != 0) ? 0xE0 : -0xE0));
        }
        if ((u16)b32 & DH_PAD_Z_POS) {                           /* tst #128 -> 0x80 */
            M32(e, 0x80) = (ACTOR_WORK(e)->facing_right != 0) ? -0x600 : 0x600;
        } else if ((u16)b32 & DH_PAD_Z_NEG) {
            M32(e, 0x80) = (ACTOR_WORK(e)->facing_right != 0) ? 0x600 : -0x600;
        } else {
            M32(e, 0x80) = 0;
        }
        {
            s16 cc = (s16)(M16(e, 0xCC) - 1);
            M16(e, 0xCC) = cc;
            if (cc == 0x14) {
                M32(e, 0xC4) = 0;
                if (st == 0x1A) {
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A98u, M8(e, 0x45) * 8);
                }
            }
        }
        if (M32(e, 0xC4) != 0)
            M16(e, 0xCE) = (s16)(M16(e, 0xCE) - 1);

        switch (st) {                                    /* switch 6 */
        case 27:
            if ((s32)ACTOR_U16(e, animation_frame) < 6) {
                if (PLAYER_INPUT(rec)->button1_hold_frames >= 5) {
                    st = 0x1D;
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087B14u + M8(e, 0x45) * 4), (slot - 1) * 2);
                }
                if (M32(e, 0x7C) < 0) {
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A9Au, M8(e, 0x45) * 8);
                    PLAYER_INPUT(rec)->direction_hold_frames = 0;
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    cat = 0x8000;
                    st  = 0x22;
                    M32(e, 0xC4) = 3;
                }
            } else {
                st = 0x1D;
            }
            break;
        case 28:
            if ((s32)ACTOR_U16(e, animation_frame) < 6) {
                if (PLAYER_INPUT(rec)->button2_hold_frames >= 5) {
                    ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087B74u + M8(e, 0x45) * 4), (slot - 1) * 2);
                    st = 0x1D;
                    ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                } else if (M32(e, 0x7C) < 0) {
                    ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A9Au, M8(e, 0x45) * 8);
                    PLAYER_INPUT(rec)->direction_hold_frames = 0;
                    M32(e, 0x80) = 0;
                    ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                    cat = 0x8000;
                    st  = 0x22;
                    M32(e, 0xC4) = 3;
                }
            } else {
                st = 0x1D;
            }
            break;
        case 29:
            if (M32(e, 0x7C) < 0) {
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A9Au, M8(e, 0x45) * 8);
                PLAYER_INPUT(rec)->direction_hold_frames = 0;
                M32(e, 0x80) = 0;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                cat = 0x8000;
                st  = 0x22;
                M32(e, 0xC4) = 3;
            }
            break;
        case 30:
            if (M32(e, 0x7C) < 0) {
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087BD0u, M8(e, 0x45) * 2);
                PLAYER_INPUT(rec)->direction_hold_frames = 0;
                M32(e, 0x80) = 0;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                cat = 0x8000;
                st  = 0x25;
            }
            break;
        case 31:
            if (M32(e, 0x7C) < 0) {
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087BE6u, M8(e, 0x45) * 2);
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                cat = 0x8000;
                st  = 0x25;
            }
            break;
        case 32:
            if ((s32)ACTOR_U16(e, animation_frame) <= 0xC) {
                if (((u16)b30 & DH_PAD_ACTION1) && ((s32)ACTOR_U16(e, animation_frame) >= 6))
                    ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            } else {
                st = 0x1D;
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A98u, M8(e, 0x45) * 8);
            }
            /* FALLTHROUGH: state 32 shares the case-33 tail (block_353 @0x22437c) */
            __attribute__((fallthrough));
        case 33:
            if (M32(e, 0x7C) < 0) {
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A9Au, M8(e, 0x45) * 8);
                PLAYER_INPUT(rec)->direction_hold_frames = 0;
                M32(e, 0x80) = 0;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                cat = 0x8000;
                st  = 0x22;
                M32(e, 0xC4) = 3;
            }
            break;
        case 26:
            if ((u16)b30 & 3) {
                ACTOR_WORK(e)->action_category = (s32)(ACTOR_WORK(e)->action_category | 0x400);
                if ((u16)PLAYER_INPUT(rec)->buttons & 0x40) {
                    if ((u16)b30 & DH_PAD_ACTION1) {
                        st = 0x1E;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087BA4u, M8(e, 0x45) * 2);
                    } else {
                        st = 0x1F;
                        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087BBAu, M8(e, 0x45) * 2);
                    }
                } else if ((u16)b30 & DH_PAD_ACTION1) {
                    if (M8(e, 0x45) == 1)      st = 0x20;
                    else if (M8(e, 0x45) == 2) st = 0x21;
                    else                        st = 0x1B;
                    ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087AE4u + M8(e, 0x45) * 4), (slot - 1) * 2);
                } else {
                    st = 0x1C;
                    ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087B44u + M8(e, 0x45) * 4), (slot - 1) * 2);
                }
                ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            } else if (M32(e, 0x7C) < 0) {
                ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A9Au, M8(e, 0x45) * 8);
                PLAYER_INPUT(rec)->direction_hold_frames = 0;
                ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
                cat = 0x8000;
                st  = 0x22;
                M32(e, 0xC4) = 3;
            }
            break;
        }
    }

    /* ===== special-move init (switch 8), cat & 0x037FBFF0 ===== */
    if (cat & 0x037FBFF0u) {
        /* none */
    } else if (M8((void *)0x06087976u, M8(e, 0x45) * 4) == 0) {
        /* none */
    } else if (!((u16)b30 & DH_PAD_ACTION3)) {
        /* none */
    } else {
        switch ((u16)b30 & 3) {
        case 2:
            if ((u16)b32 & DH_PAD_X_NEG) {
                M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0) ? -0x2000 : 0x2000;
                M32(e, 0x98) = 1;
                st = 0x36;
                M16(e, 0xCC) = 0x11;
                M16(e, 0xCE) = 0x1D;
                M32(e, 0xC8) = (s32)((M16(e, 0xCE) * 2) - 0xC0);
            } else {
                M32(e, 0x78) = 0;
                M32(e, 0x98) = 0;
                st = 0x35;
                M16(e, 0xCC) = 0x1E;
                M16(e, 0xCE) = 0x21;
                M32(e, 0xC8) = (s32)((M16(e, 0xCE) * 2) - 0x110);
            }
            ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087C80u + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
            cat = 0x100400;
            ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            for (i = 0; i < 2; i++)
                M16((void *)(uintptr_t)(0x06096CB0u + (s16)((s16)slot * 0x3C) + (s8)(i * 6)), 0)
                    = (s16)M32((void *)0x06096C74u, slot * 4);
            M16((void *)0x06096CA0u, slot * 2) = 0;
            M16((void *)0x06096E90u, slot * 2) = 0;
            M16((void *)0x06096EA0u, slot * 2) = 0;
            ACTOR_M16(e, interaction_latch) = 1;
            break;
        case 1:
            if ((u16)b32 & DH_PAD_X_NEG) {
                M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0) ? -0x2000 : 0x2000;
                M32(e, 0x98) = 1;
                st = 0x32;
                M16(e, 0xCC) = 0x11;
                M16(e, 0xCE) = 0x1D;
                M32(e, 0xC8) = (s32)((M16(e, 0xCE) * 2) - 0xC0);
            } else {
                M32(e, 0x78) = 0;
                M32(e, 0x98) = 0;
                st = 0x31;
                M16(e, 0xCC) = 0x1E;
                M16(e, 0xCE) = 0x21;
                M32(e, 0xC8) = (s32)((M16(e, 0xCE) * 2) - 0x110);
            }
            ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06087BFCu + M8(e, 0x45) * 4), M32(e, 0x98) * 2);
            cat = 0x100400;
            ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            for (i = 0; i < 2; i++)
                M16((void *)(uintptr_t)(0x06096CB0u + (s16)((s16)slot * 0x3C) + (s8)(i * 6)), 0)
                    = (s16)M32((void *)0x06096C74u, slot * 4);
            M16((void *)0x06096CA0u, slot * 2) = 0;
            M16((void *)0x06096E90u, slot * 2) = 0;
            M16((void *)0x06096EA0u, slot * 2) = 0;
            ACTOR_M16(e, interaction_latch) = 1;
            break;
        }
    }

    /* ===== jump-start init, cat & 0x037FBF70 ===== */
    if (cat & 0x037FBF70u) {
        /* none */
    } else if (!((u16)b30 & DH_PAD_ACTION3)) {
        /* none */
    } else if (M8((void *)0x06087976u, M8(e, 0x45) * 4) == 0) {
        /* none */
    } else {
        ACTOR_WORK(e)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_WORK(e)->animation_requested = M16((void *)0x06087A94u, M8(e, 0x45) * 8);
        M16(e, 0xCC) = 0x1E;
        M16(e, 0xCE) = 0x1D;
        M32(e, 0xC8) = (s32)((M16(e, 0xCE) * 2) - 0x130);
        M32(e, 0xC4) = 1;
        if ((u16)b32 & DH_PAD_X_NEG) {
            if (cat & 0x80) {                            /* tst #128 -> 0x80 */
                M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0) ? -0x2C00 : 0x2C00;
            } else {
                M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0) ? -0x1500 : 0x1500;
            }
        } else if ((u16)b32 & DH_PAD_X_POS) {
            M32(e, 0x78) = (ACTOR_WORK(e)->facing_right != 0) ? 0x1000 : -0x1000;
        } else {
            M32(e, 0x78) = 0;
        }
        cat = 0x8000;
        st  = 0x18;
        ACTOR_M16(e, interaction_latch) = 1;
    }

    /* ===== button 0x40/0x47 edge detect ===== */
    if ((u16)b32 & DH_PAD_X_NEG) {
        cat |= 2;
        M8(e, 0x47) = 1;
    } else {
        if ((M8(e, 0x46) == 0) && ((u16)b32 & DH_PAD_Z_POS) && !((u16)b32 & DH_PAD_HORIZONTAL)) {
            M8(e, 0x46) = 6;
        } else if ((M8(e, 0x46) == 6) && !((u16)b32 & DH_PAD_Z_POS)) {
            M8(e, 0x46) = 0;
        }
        cat &= ~2;
        M8(e, 0x47) = 0;
    }

L_combo:
    /* ===== 0x225c12: emit buffered interaction records via func_02225fac ===== */
    if (ACTOR_M16(e, interaction_latch) != 0) {
        /* overflow flag set: skip */
    } else {
        arr_base = MP(MP((void *)0x060862A8u, slot * 4), M8(e, 0x45) * 4);
        arr_base = MP(arr_base, (M8(e, 0x46) + M8(e, 0x47)) * 4);
        src_desc = (void *)((u8 *)arr_base + MU16((void *)0x06096EA0u, slot * 2) * 0xC);
        interaction_append_slot_records(e, src_desc, arr_base, b30);
    }

    /* ===== 0x225f48: per-slot replay-buffer bookkeeping ===== */
    if (MU16((void *)0x06096E90u, slot * 2) != 0) {
        if (((s32)MU16((void *)0x06096E90u, slot * 2) + 1) < M16((void *)0x06096CA0u, slot * 2)) {
            for (i = 0; i < 10; i++)
                M16((void *)(uintptr_t)(0x06096CB0u + (s16)((s16)slot * 0x3C) + (s8)(i * 6)), 0)
                    = (s16)M32((void *)0x06096C74u, slot * 4);
            ACTOR_M16(e, interaction_latch) = 0;
            M8(e, 0x47)  = 0;
            M8(e, 0x46)  = 6;
            M8(e, 0x46)  = 0;
            M16((void *)0x06096CA0u, slot * 2) = 0;
            M16((void *)0x06096E90u, slot * 2) = 0;
            M16((void *)0x06096EA0u, slot * 2) = 0;
            cat &= ~0x402;
            M16(rec2, 2) = 0;
            if (cat & 0x230000)
                goto b481;
            cat |= 1;
            if (actor_step_effect_transition(e) == 0)
                goto b481;
            cat = ACTOR_WORK(e)->action_category;
            goto L_commit;
        } else {
            if (MU16((void *)0x06096CA0u, slot * 2) == 0) {
                ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                M16((void *)0x06096CA0u, slot * 2) =
                    (s16)(M16((void *)0x06096CA0u, slot * 2) + 1);
                cat = (cat | 0x400) & ~0xD;
            } else if (ACTOR_WORK(e)->animation_events & 3) {
                if (ACTOR_WORK(e)->animation_events & 2) {
                    if ((s32)MU16((void *)(uintptr_t)(0x06096CB0u + (s16)((s16)slot * 0x3C)
                                           + (s8)(M16((void *)0x06096CA0u, slot * 2) * 6)), 6)
                        <= 0x22)
                        M16((void *)0x06096EB0u, slot * 2) = 0;
                } else {
                    M16((void *)0x06096CA0u, slot * 2) =
                        (s16)(M16((void *)0x06096CA0u, slot * 2) + 1);
                    M16((void *)0x06096EB0u, slot * 2) = 0;
                }
            } else if (((s32)MU16((void *)0x06096E90u, slot * 2)
                            >= M16((void *)0x06096CA0u, slot * 2)) &&
                       ((u32)ACTOR_U16(e, animation_frame)
                            > (u32)MU16((void *)(uintptr_t)(0x06096CB0u + (s16)((s16)slot * 0x3C)
                                       + (s8)(M16((void *)0x06096CA0u, slot * 2) * 6)), 2)) &&
                       (ACTOR_WORK(e)->animation_control == 0)) {
                ACTOR_WORK(e)->animation_control = DH_ANIMATION_CONTROL_RESTART;
                if (((u16)b32 & DH_PAD_X_POS) && !(cat & 0x20000)) {
                    ACTOR_WORK(e)->angle_y = (s16)(ACTOR_WORK(e)->angle_y + 0x8000);
                    ACTOR_WORK(e)->facing_right  = (s8)(ACTOR_WORK(e)->facing_right ^ 1);
                }
            }
        b481:
            ACTOR_WORK(e)->animation_requested = M16((void *)(uintptr_t)(0x06096CB0u + (s16)((s16)slot * 0x3C)
                               + (s8)(M16((void *)0x06096CA0u, slot * 2) * 6)), 0);
            if ((cat & 0x400) && !(cat & 0x20000)) {
                if ((u16)b32 & DH_PAD_Z_POS) {                    /* tst #128 -> 0x80 */
                    ACTOR_WORK(e)->movement_z = (s32)(ACTOR_WORK(e)->movement_z + ((ACTOR_WORK(e)->facing_right != 0) ? -0x400 : 0x400));
                } else if ((u16)b32 & DH_PAD_Z_NEG) {
                    ACTOR_WORK(e)->movement_z = (s32)(ACTOR_WORK(e)->movement_z + ((ACTOR_WORK(e)->facing_right != 0) ? 0x400 : -0x400));
                }
            }
        }
    }

L_commit:
    /* ===== epilogue: commit working copies, tick stale counter ===== */
    ACTOR_WORK(e)->action_category = cat;
    ACTOR_WORK(e)->action_state = st;
    ACTOR_WORK(e)->animation_events = 0;
    if (!(M8(e, 0x36) & M32((void *)DH_FRAME_TICK_ADDR_U, 0))) {
        M16((void *)0x06096EB0u, slot * 2) =
            (s16)(M16((void *)0x06096EB0u, slot * 2) + 1);
        if (M16((void *)0x06096EB0u, slot * 2) > 0x1F)
            M16((void *)0x06096EB0u, slot * 2) = 0x1F;
    }
}

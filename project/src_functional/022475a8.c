/* enemy_update_dash [0x022475a8]: Advances dash timers, re-aim loops and recovery before animation. */
#include "dh_work.h"
#include "dh.h"
/* 0x022475a8  per-frame update for the dash/charge move. arg0 = task/ctx (passed
 * to the animation_update tail), actor = arg1, rec = MP(actor,0x100) the move
 * record, mayact = MU16(actor,0x54) & 2. State on M32(actor,0xD8):
 *   0 arm (needs mayact): go to state 1, heading rec.unkE (& 0x1FFF, flag 0x4000
 *     -> unkE0 bit 0x08000000), load the step timers unkC4/unkC8/unkCC from
 *     rec.unk10/11/12.
 *   1 run: tick unkC4; on 0 re-aim to rec.unk14 (looping unkCC times by unkC8);
 *     latch velocity (unk78=unkEC, unk80=unkF4); apply gravity to unkF0 (always
 *     when rec.unk13==1, else only until -0x2000); while airborne/forward store
 *     unk7C, else land -> state 2 (stop, set unk84 bits, heading rec.unk16, clear
 *     accumulators).
 *   2 land resolve (needs mayact): branch on rec.unk13:
 *     0 -> if the target slot g_060995b0[unk120] is live, close enough
 *          (func_02245fc4) and within the depth budget, either continue facing
 *          (rec.unk1A, via actor_set_state_7) or turn (state 3, heading rec.unk18);
 *          0xFFFF entries fall back to func_022469f2.
 *     1 -> func_02249240 then reset unkC4 (returns without the draw step).
 *     2 -> same gates as 0 but using rec.unk1A/unk18/unk14 unconditionally.
 *   3 end (needs mayact) -> func_022469f2.
 * All paths except state 2 / rec.unk13==1 finish with animation_update.
 *
 * bounds: [0x022475a8, 0x247924); split out of func_022473ee's over-swallowed
 *   extent. notes: both switches are cmp/eq chains; func_02210020 takes no args.
 * fidelity: HIGH - nested control flow and widths verified against fn.py --asm. */
extern s32  g_actor_slot_ids[];      /* per-slot active flag */
extern void *g_actor_slot_tasks_legacy[];     /* per-slot anchor pointers; deref +8 -> actor */

s32  actor_anim_frame_update(void *ctx, void *actor);      /* 0x0222dc2c animation_update */
void enemy_reset_idle_or_walk(void *actor);                 /* 0x022469f2 */
void actor_set_state_7(void *actor);                 /* 0x0224840a actor_set_state_7 */
void enemy_enter_timed_reaction(void *actor);                 /* 0x02249240 */
s32  enemy_classify_attack_timing(void *actor, void *target);   /* 0x02245fc4 reach test */
s32  game_random_next(void);                        /* 0x02210020 rng */

void enemy_update_dash(void *ctx, void *actor) {
    void *rec = ACTOR_MP(actor, queued_attack_kind);
    s32 mayact = ((u16)ACTOR_MU16(actor, animation_events)) & 2;

    switch (ACTOR_M32(actor, behavior_phase)) {
    case 0:
        if (mayact != 0) {
            s16 w = M16(rec, 0xE);
            ACTOR_M32(actor, behavior_phase) = 1;
            ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_M32(actor, action_category) = 0x10000;
            ACTOR_M16(actor, animation_requested) = (s16)(0x1FFF & w);
            if (((u16)w) & 0x4000) {
                ACTOR_M32(actor, state_flags) |= 0x08000000;
            } else {
                ACTOR_M32(actor, state_flags) &= 0xF7FFFFFF;
            }
            M32(actor, 0xC4) = (u8)MU8(rec, 0x10);
            M32(actor, 0xC8) = (u8)MU8(rec, 0x11);
            M16(actor, 0xCC) = (s16)(u8)MU8(rec, 0x12);
        }
        break;
    case 1: {
        s32 c4 = M32(actor, 0xC4) - 1;
        s32 nx, grav;
        M32(actor, 0xC4) = c4;
        if (c4 == 0) {
            ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
            if (M32(actor, 0xC8) == 0) {
                ACTOR_M16(actor, animation_requested) = M16(rec, 0x14);
            } else {
                s16 cc = M16(actor, 0xCC) - 1;
                M16(actor, 0xCC) = cc;
                if (cc > 0) {
                    M32(actor, 0xC4) = M32(actor, 0xC8);
                }
                ACTOR_M16(actor, animation_requested) = M16(rec, 0x14);
            }
        }
        M32(actor, 0x78) = M32(actor, 0xEC);
        M32(actor, 0x80) = M32(actor, 0xF4);
        nx = ACTOR_M32(actor, position_y) + M32(actor, 0xF0);
        grav = ACTOR_M32(actor, queued_attack_slot);
        if ((u8)MU8(rec, 0x13) != 1) {
            if (M32(actor, 0xF0) > -0x2000) {
                M32(actor, 0xF0) -= grav;
            }
        } else {
            M32(actor, 0xF0) -= grav;
        }
        if (M32(actor, 0xF0) >= 0 || nx > 0) {
            M32(actor, 0x7C) = nx;
        } else {
            ACTOR_M32(actor, position_y) = 0;
            ACTOR_M32(actor, behavior_phase) = 2;
            ACTOR_M32(actor, action_category) &= 0xFFFEFFFF;
            ACTOR_M32(actor, action_category) |= 0x8201;
            ACTOR_M32(actor, state_flags) &= 0x08000300;
            ACTOR_M32(actor, action_phase) = 0;
            ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_M32(actor, state_flags) &= 0xF7FFFFFF;
            ACTOR_M16(actor, animation_requested) = M16(rec, 0x16);
            ACTOR_M32(actor, movement_x) = 0;
            ACTOR_M32(actor, movement_y) = 0;
            ACTOR_M32(actor, movement_z) = 0;
        }
        break;
    }
    case 2: {
        s32 slot;
        if (mayact == 0) {
            break;
        }
        slot = ACTOR_M32(actor, target_slot);
        switch ((u8)MU8(rec, 0x13)) {
        case 0:
            if (g_actor_slot_ids[slot] != 0) {
                void *target = TASK_PTR(g_actor_slot_tasks_legacy[slot], work);
                if ((ACTOR_M32(actor, state_flags) & 0x100) &&
                    ((u32)(game_random_next() & 7) <= (u32)M32(actor, 0xE8)) &&
                    enemy_classify_attack_timing(actor, target) == 0) {
                    if ((1 & (M32(actor, 0x124) >> 1)) == ACTOR_M8(actor, facing_right)) {
                        if ((u16)MU16(rec, 0x1A) == 0xFFFF) {
                            enemy_reset_idle_or_walk(actor);
                        } else {
                            actor_set_state_7(actor);
                            ACTOR_M16(actor, animation_requested) = M16(rec, 0x1A);
                        }
                    } else if ((u16)MU16(rec, 0x18) == 0xFFFF) {
                        enemy_reset_idle_or_walk(actor);
                    } else {
                        ACTOR_M32(actor, behavior_phase) = 3;
                        ACTOR_M32(actor, state_flags) &= 0x300;
                        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                        ACTOR_M16(actor, animation_requested) = M16(rec, 0x18);
                    }
                } else {
                    enemy_reset_idle_or_walk(actor);
                }
            } else {
                enemy_reset_idle_or_walk(actor);
            }
            break;
        case 1:
            enemy_enter_timed_reaction(actor);
            M32(actor, 0xC4) = 0;
            return;
        case 2:
            if (g_actor_slot_ids[slot] == 0) {
                enemy_reset_idle_or_walk(actor);
            } else {
                void *target = TASK_PTR(g_actor_slot_tasks_legacy[slot], work);
                if ((ACTOR_M32(actor, state_flags) & 0x100) &&
                    ((u32)(game_random_next() & 7) <= (u32)M32(actor, 0xE8)) &&
                    enemy_classify_attack_timing(actor, target) == 0) {
                    if ((1 & (M32(actor, 0x124) >> 1)) == ACTOR_M8(actor, facing_right)) {
                        actor_set_state_7(actor);
                        ACTOR_M16(actor, animation_requested) = M16(rec, 0x1A);
                    } else {
                        ACTOR_M16(actor, animation_requested) = M16(rec, 0x18);
                        ACTOR_M32(actor, behavior_phase) = 3;
                        ACTOR_M32(actor, state_flags) &= 0x300;
                        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                    }
                } else {
                    ACTOR_M16(actor, animation_requested) = M16(rec, 0x14);
                    ACTOR_M32(actor, behavior_phase) = 3;
                    ACTOR_M32(actor, state_flags) &= 0x300;
                    ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                }
            }
            break;
        default:
            break;
        }
        break;
    }
    case 3:
        if (mayact != 0) {
            enemy_reset_idle_or_walk(actor);
        }
        break;
    }

    actor_anim_frame_update(ctx, actor);
}

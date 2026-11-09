#include "dh_work.h"
/* enemy_update_combo [0x022492f8]: Runs the fourteen-phase combat combo machine and its animation continuation. */
#include "dh.h"
/* 0x022492f8  actor_combo_machine - 14-state combat-combo state machine.
 * ABI (r4=task, r5=entity). animation_update (actor_anim_frame_update 0x0222dc2c) is reached
 * through r12 (jsr @r12 / braf tail); ptr_resolve: mova 0x249320 + 0xfffe490c =
 * 0x0222dc2c. Prologue consts: r6=0xffffdfff(~0x2000 mask), r7=8, r8=7, r9=1,
 * r10=3, r11=0. Prologue also latches two working values used across the states:
 *     ec       = entity[0xec]          (stored into entity[0x14] by states 2/9/10)
 *     engaged2 = entity[0x54] & 2      (read by states 11/12/13)
 *
 * Dispatch: if ((u32)entity[0xd8] >= 14) return; else runtime braf table @0x249354
 * (14 x s16, braf PC 0x249344, target = 0x249348 + (s16)tbl[state]). Table decoded
 * and verified from ROM bytes (matches the deferral note):
 *    0 -> 0x249370   1 -> 0x249404   2 -> 0x24941e   3 -> 0x2494a4
 *    4 -> 0x2494da   5 -> 0x249590   6 -> 0x2495e0   7 -> 0x249600
 *    8 -> 0x24965a   9 -> 0x2496d8  10 -> 0x24972a  11 -> 0x249788
 *   12 -> 0x24979c  13 -> 0x2497ec
 *
 * Two shared epilogues: .L24981c issues animation_update then returns; .L249822 is
 * the bare return (used where animation_update was already issued this tick). States
 * 2 and 10 share the wrap tail .L249742 (entity[0x18]=entity[0xf0]; anim; ret).
 * State 4 has a SECONDARY cmp-chain sub-dispatch on func_022454e4()'s return (0..6).
 * State 0's combo sub-tables: 0x060540e8 (s8[] by entity[0x5e]), 0x06054080 (8-byte
 * records), 0x06053f04 (s16[]), 0x060e0ddc (enable flag) - identical shape to the
 * 0x248bb8 handler's combo block.
 *
 * All computed branches resolved with braf_resolve (trusting target addresses); the
 * in-range bsrf callees + the two bra/bsr tails to sibling handlers (0x249240
 * actor_enter_state4, 0x249cda actor_enter_despawn, 0x2499a6 actor_enter_state14)
 * all confirmed. actor_anim_frame_update is declared returning s32 because state 9 branches on
 * its return value. fidelity: HIGH - hand-traced statement-by-statement vs fn.py. */
extern s32  actor_anim_frame_update(void *task, void *entity);   /* animation_update */
extern void sound_queue_push(s32 arg);
extern s32  enemy_select_pending_reaction(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern s32  enemy_choose_and_start_motion(void *entity);
extern s32  actor_script_step(void *entity);
extern s32  enemy_test_player_pair_lock(void *entity);
extern void enemy_enter_optional_pose(void *entity);               /* actor_enter_state14 */
extern void enemy_enter_timed_reaction(void *entity);               /* actor_enter_state4 */
extern void enemy_enter_despawn(void *task, void *entity);   /* actor_enter_despawn */
extern s32  actor_decode_enemy_direction(void *entity);
extern void actor_load_transition_pose(void *entity);
extern void actor_begin_effect_action_one(void *entity);
extern void enemy_load_action_descriptor(void *entity);
extern void enemy_enter_timed_state16(void *entity);

void enemy_update_combo(void *task, void *entity) {
    s32 ec       = M32(entity, 0xEC);
    s32 engaged2 = 2 & (s32)ACTOR_MU16(entity, animation_events);

    if ((u32)ACTOR_M32(entity, behavior_phase) >= 14) {
        return;                                 /* .L249822 bare return */
    }

    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0:                                      /* 0x249370 */
        actor_anim_frame_update(task, entity);
        if ((u16)ACTOR_MU16(entity, animation_events) != 2) {
            return;
        }
        if (ACTOR_M16(entity, health) > 0) {            /* 0x2493de */
            if (ACTOR_M32(entity, action_phase) == 6 && enemy_select_pending_reaction(entity) != 0) {
                actor_anim_frame_update(task, entity);    /* .L24981c */
                return;
            }
            enemy_reset_idle_or_walk(entity);              /* 0x2493f4 */
            actor_anim_frame_update(task, entity);
            return;
        } else {                                /* 0x24938c combo-table block */
            s16 sel = ACTOR_M16(entity, model_index);
            s8  ta  = ((s8 *)(uintptr_t)0x060540E8)[sel];
            s8 *rec = (s8 *)(uintptr_t)(0x06054080 + (s32)ta * 8);
            if (*(s32 *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR != 0) {
                s16 tb = ((s16 *)(uintptr_t)0x06053F04)[(u8)rec[1]];
                sound_queue_push((s32)(((u32)((s32)tb << 8)) | DH_SOUND_CUE_BASE_U));
            }
            ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_MU16(entity, animation_requested) = 0x13D;
            ACTOR_M32(entity, behavior_phase) = 3;
            ACTOR_M32(entity, state_flags) = 0x40C00;
            ACTOR_M32(entity, action_phase) = 1;
            M32(entity, 0xBC) = -1;
            return;                             /* .L249822 bare return */
        }

    case 1:                                      /* 0x249404 */
        actor_anim_frame_update(task, entity);
        if (ACTOR_MU16(entity, animation_events) & 1) {
            ACTOR_M32(entity, behavior_phase) = 3;
        }
        return;

    case 2: {                                    /* 0x24941e */
        s32 sum;
        ACTOR_M32(entity, movement_x) = ec;
        ACTOR_M32(entity, movement_z) = M32(entity, 0xF4);
        sum = ACTOR_M32(entity, position_y) + M32(entity, 0xF0);
        M32(entity, 0xF0) -= 0x400;
        if (sum >= -49152) {                    /* .L249742 wrap tail */
            ACTOR_M32(entity, movement_y) = M32(entity, 0xF0);
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        ACTOR_M32(entity, position_y) = 0;                   /* 0x249440 */
        ACTOR_M32(entity, behavior_phase) = 8;
        ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_MU16(entity, animation_requested) += 1;
        ACTOR_M32(entity, action_category) &= ~0x2000;
        ACTOR_M32(entity, action_phase) = 3;
        M32(entity, 0xBC) = 0x7FFFFFFF;
        ACTOR_M32(entity, movement_x) = 0;
        ACTOR_M32(entity, movement_y) = 0;
        ACTOR_M32(entity, movement_z) = 0;
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;
    }

    case 3:                                      /* 0x2494a4 */
        actor_anim_frame_update(task, entity);
        if (!(ACTOR_MU16(entity, animation_events) & 2)) {
            return;
        }
        if (ACTOR_M16(entity, health) > 0) {
            enemy_enter_timed_reaction(entity);              /* 0x2494c4 tail */
        } else {
            enemy_enter_despawn(task, entity);        /* 0x249676 tail */
        }
        return;

    case 4: {                                    /* 0x2494da */
        s32 c = M32(entity, 0xC4);
        s32 sel;
        M32(entity, 0xC4) = c - 1;
        if (c > 0) {
            return;                             /* .L249822 bare return */
        }
        sel = enemy_choose_and_start_motion(entity);
        switch (sel) {
        case 0:                                 /* 0x249514 */
        case 1:
        case 3:
        case 4:
            ACTOR_M32(entity, action_category) = 0x804000;
            if (ACTOR_M32(entity, state_flags) & 0x10000) {
                ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_TURN;
                ACTOR_M32(entity, behavior_phase) = 0;
                ACTOR_M32(entity, action_category) |= 16;
            } else {                            /* 0x249538 */
                ACTOR_M32(entity, behavior_phase) = 7;
            }
            break;
        case 2:                                 /* 0x24953e */
            ACTOR_M32(entity, behavior_phase) = 5;
            ACTOR_M32(entity, action_category) = 0x804000;
            break;
        case 5:                                 /* 0x24954a */
            if (ACTOR_M8(entity, saved_primary_effect_type) >= 3) {
                ACTOR_M32(entity, behavior_phase) = 11;
            } else {
                ACTOR_M32(entity, behavior_phase) = 12;
            }
            ACTOR_M32(entity, action_category) = 0x804000;
            break;
        case 6:                                 /* 0x249566 */
            ACTOR_M32(entity, behavior_phase) = 11;
            ACTOR_M32(entity, action_category) = 0x804000;
            break;
        default:
            return;                             /* .L249822 bare return */
        }
        actor_anim_frame_update(task, entity);            /* 0x249570 anim + ret */
        return;
    }

    case 5:                                      /* 0x249590 */
        if (actor_script_step(entity) != 1) {
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        ACTOR_M32(entity, behavior_phase) = 6;
        ACTOR_M32(entity, action_phase) = 4;
        M32(entity, 0xBC) = M32(entity, 0x140);
        if (!(ACTOR_M32(entity, state_flags) & 0x10000)) {
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        ACTOR_M8(entity, facing_right) ^= 1;
        ACTOR_MU16(entity, angle_y) += 0x8000;
        ACTOR_M32(entity, state_flags) &= ~0x10000;
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;

    case 6:                                      /* 0x2495e0 */
        if (actor_script_step(entity) != 1) {
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        ACTOR_M32(entity, behavior_phase) = 7;
        ACTOR_M32(entity, action_phase) = 3;
        M32(entity, 0xBC) = 0x7FFFFFFF;
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;

    case 7:                                      /* 0x249600 */
        if (actor_script_step(entity) != 0) {
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        M32(entity, 0xBC) = M32(entity, 0x140);
        if (ACTOR_M32(entity, state_flags) & 0x20000) {
            ACTOR_M8(entity, facing_right) ^= 1;
            ACTOR_MU16(entity, angle_y) += 0x8000;
            enemy_reset_idle_or_walk(entity);
        } else {                                /* 0x24963e */
            if (enemy_test_player_pair_lock(entity) != 0) {
                enemy_enter_optional_pose(entity);          /* actor_enter_state14 */
            } else {
                enemy_reset_idle_or_walk(entity);
            }
        }
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;

    case 8:                                      /* 0x24965a */
        actor_anim_frame_update(task, entity);
        if (!(ACTOR_MU16(entity, animation_events) & 2)) {
            return;
        }
        if (ACTOR_M16(entity, health) > 0) {
            enemy_enter_timed_reaction(entity);              /* 0x24968e tail */
        } else {
            enemy_enter_despawn(task, entity);        /* 0x249676 tail */
        }
        return;

    case 9:                                      /* 0x2496d8 */
        ACTOR_M32(entity, movement_x) = ec;
        if (actor_anim_frame_update(task, entity) == 0) {
            return;                             /* .L249822 bare return */
        }
        ACTOR_M32(entity, behavior_phase) = 10;                 /* 0x2496e8 */
        ACTOR_M8(entity, action_pending) = 0;
        M32(entity, 0xF0) = 0x3000;
        M32(entity, 0xBC) = M32(entity, 0x140) | 0x7FFFFF00;
        ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        if (M32(entity, 0xEC) > 0) {
            M32(entity, 0xEC) = -0x2000;
            ACTOR_MU16(entity, animation_requested) = 0x143;
        } else {                                /* 0x24971c */
            M32(entity, 0xEC) = 0x2000;
            ACTOR_MU16(entity, animation_requested) = 0x145;
        }
        return;                                 /* .L249822 bare return */

    case 10: {                                   /* 0x24972a */
        s32 sum;
        ACTOR_M32(entity, movement_x) = ec;
        sum = ACTOR_M32(entity, position_y) + M32(entity, 0xF0);
        M32(entity, 0xF0) -= 0x400;
        if (sum >= -49152) {                    /* .L249742 wrap tail */
            ACTOR_M32(entity, movement_y) = M32(entity, 0xF0);
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        ACTOR_M32(entity, position_y) = 0;                   /* 0x24974a */
        ACTOR_M32(entity, behavior_phase) = 8;
        ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
        if ((u16)ACTOR_MU16(entity, animation_current) == 0x143) {
            ACTOR_MU16(entity, animation_requested) = 0x132;
        } else {                                /* 0x249768 */
            ACTOR_MU16(entity, animation_requested) = 0x12F;
        }
        ACTOR_M32(entity, action_category) &= ~0x2000;
        ACTOR_M32(entity, action_phase) = 3;
        M32(entity, 0xBC) = 0x7FFFFFFF;
        ACTOR_M32(entity, movement_x) = 0;
        ACTOR_M32(entity, movement_y) = 0;
        ACTOR_M32(entity, movement_z) = 0;
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;
    }

    case 11:                                     /* 0x249788 */
        if (!engaged2) {
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        enemy_reset_idle_or_walk(entity);
        M32(entity, 0xBC) = M32(entity, 0x140);
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;

    case 12:                                     /* 0x24979c */
        if (!engaged2) {
            actor_anim_frame_update(task, entity);        /* .L24981c */
            return;
        }
        ACTOR_M32(entity, behavior_phase) = 13;
        actor_decode_enemy_direction(entity);
        actor_load_transition_pose(entity);
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;

    case 13:                                     /* 0x2497ec */
        if ((s32)ACTOR_MU16(entity, animation_frame) == ACTOR_M32(entity, action_ticks) && ACTOR_M8(entity, interpolation_ticks) == 0) {
            actor_begin_effect_action_one(entity);
            enemy_load_action_descriptor(entity);
        } else if (engaged2) {                  /* 0x249812 */
            enemy_enter_timed_state16(entity);
        }
        actor_anim_frame_update(task, entity);            /* .L24981c */
        return;

    default:
        return;
    }
}

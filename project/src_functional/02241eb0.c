/* enemy_tick_chase_phases [0x02241eb0]: Nine behavior phases combine player-side metric, animation events and movement decisions. */
#include "dh_work.h"
#include "dh.h"
/* 0x02241eb0  per-actor movement/chase state machine, dispatched on state
 * M32(actor,0xD8) (cases 0..8). Entry r4 = ctx (passed straight to the tail
 * animation_update), r5 = actor record.
 *
 * Pre-step reads: metric = func_02245a6a(actor); f54 = MU16(actor,0x54) & 2
 * (the "may-act" gate); f58 = MU16(actor,0x58) (current heading word) and
 * f90 = M32(actor,0x90) (target heading) compared for "arrived".
 *
 * States: 0 kick-off (pick 1), 1 chase/approach (the big block: step the two
 * countdowns, else measure dx/dy to slot g_060995b0[(metric&1)+1]'s inner actor
 * and either turn to face (unkD8=3) or hand off), 2/6 cruise, 3 turning,
 * 4/8 kick helpers (func_022469f2), 5 arrive+react, 7 settle.
 * Every path falls through to the tail animation_update(ctx, actor).
 *
 * bounds: [0x241eb0, 0x242226); a 14-byte literal pool precedes the entry at
 *   0x241e82 (census mis-seg'd it as the entry). The 0x3c2 fn.extent over-swallows
 *   the next function func_02242226 (split at the braf-tail rts at 0x242224).
 * handler calls were a uniform 4-reg dispatch (r4..r7); all callees take only the
 *   actor (r5/r6/r7 dead) except func_022452be(actor,other) -> void helpers below.
 * fidelity: HIGH - switch + nested conditionals verified against m2c/--asm;
 *   masks are plain `and`/`tst rN,rN` (no tst #imm8 sign-ext hazards). */
extern s32  g_frame_tick_legacy;        /* global frame counter */
extern void *g_060e0f4c[];     /* slot-by-group table (indexed by actor->unk6F) */
extern void *g_actor_slot_tasks_legacy[];     /* slot pointers; deref +8 -> inner actor */
extern s16  g_0608dae8[];      /* 2-entry heading table ([1] == 0x0608daea) */

s32  enemy_select_player_side(void *actor);                 /* 0x02245a6a */
void actor_init_random(void *actor);                 /* 0x02241da8 */
void actor_init_rng_dir(void *actor);                 /* 0x02241dde */
void enemy_enter_descriptor_state16(void *actor);                 /* 0x02241e06 */
void enemy_reset_idle_or_walk(void *actor);                 /* 0x022469f2 */
void actor_load_recovery_pose(void *actor);                 /* 0x0223ac50 */
void actor_spawn_special_debris(void *actor);                 /* 0x0223ac9c */
void actor_load_transition_pose(void *actor);                 /* 0x0223d0c6 */
s32  actor_begin_effect_action_one(void *actor);                 /* 0x0223d0fe */
void enemy_load_action_descriptor(void *actor);                 /* 0x02245c88 */
void *collision_probe_between_actors(void *actor, void *other);   /* 0x022452be */
s32  actor_anim_frame_update(void *ctx, void *actor);      /* 0x0222dc2c animation_update */

s32 enemy_tick_chase_phases(void *ctx, void *actor) {
    u32 metric = (u32)enemy_select_player_side(actor);
    s32 f54 = ((u16)ACTOR_MU16(actor, animation_events)) & 2;
    s32 f58 = (u16)ACTOR_MU16(actor, animation_frame);
    s32 f90 = ACTOR_M32(actor, action_ticks);

    switch (ACTOR_M32(actor, behavior_phase)) {
    case 2:
        M32(actor, 0xC4) -= 1;
        if (f54) {
            actor_init_random(actor);
        }
        break;
    case 3:
        M32(actor, 0xC4) -= 1;
        if (f54) {
            void *inner;
            ACTOR_M32(actor, action_start_tick) = g_frame_tick_legacy;
            if (ACTOR_M32(actor, state_flags) & 0x10000) {
                ACTOR_M8(actor, facing_right) ^= 1;
                ACTOR_M16(actor, angle_y) += 0x8000;
                ACTOR_M32(actor, state_flags) &= 0xFFFEFFFF;
                ACTOR_M8(actor, direction_code) ^= 2;
            }
            inner = MP(g_060e0f4c[ACTOR_M8(actor, secondary_effect_slot)], 8);
            if (M16(inner, 0x56) > 0) {
                actor_init_random(actor);
            } else {
                ACTOR_M32(actor, behavior_phase) = 7;
                ACTOR_M32(actor, state_flags) &= 0xFFEFFFFF;
                actor_load_recovery_pose(actor);
            }
        }
        break;
    case 4:
        if (f54) {
            enemy_reset_idle_or_walk(actor);
            M32(actor, 0xBC) = M32(actor, 0x140);
        }
        break;
    case 5:
        if (f58 == f90 && ACTOR_M8(actor, interpolation_ticks) == 0) {
            actor_begin_effect_action_one(actor);
            enemy_load_action_descriptor(actor);
        } else if (f54) {
            enemy_reset_idle_or_walk(actor);
        }
        break;
    case 6:
        if (f58 == f90 && ACTOR_M8(actor, interpolation_ticks) == 0) {
            actor_begin_effect_action_one(actor);
        } else if (f54) {
            actor_init_random(actor);
        }
        break;
    case 7:
        if (f58 == f90) {
            actor_spawn_special_debris(actor);
            ACTOR_M32(actor, behavior_phase) = 8;
        }
        break;
    case 8:
        if (f54) {
            if (ACTOR_M8(actor, saved_primary_effect_type) < 3) {
                ACTOR_M32(actor, behavior_phase) = 6;
                ACTOR_M32(actor, state_flags) &= 0xFEFFFFFF;
                actor_load_transition_pose(actor);
                ACTOR_M32(actor, state_flags) |= 0x01000000;
            } else {
                enemy_reset_idle_or_walk(actor);
            }
        }
        break;
    case 0:
        if (M32(actor, 0xC4) < 0x1E) {
            ACTOR_M32(actor, behavior_phase) = 1;
        } else if (metric == 0) {
            ACTOR_M32(actor, behavior_phase) = 1;
            M32(actor, 0xC4) = 0;
        } else if (metric < 0x64000U) {
            ACTOR_M32(actor, behavior_phase) = 1;
            M32(actor, 0xC4) = 0x1E;
        }
        /* fallthrough */
    case 1: {
        s32 c4 = M32(actor, 0xC4) - 1;
        M32(actor, 0xC4) = c4;
        if ((c4 + 1) < 0 || metric == 0) {
            enemy_enter_descriptor_state16(actor);
        } else {
            s32 c8 = M32(actor, 0xC8) - 1;
            M32(actor, 0xC8) = c8;
            if ((c8 + 1) < 0) {
                actor_init_rng_dir(actor);
            } else {
                void *tgt = TASK_PTR(g_actor_slot_tasks_legacy[(metric & 1) + 1], work);
                s32 face;
                s32 dx = M32(tgt, 8) - ACTOR_M32(actor, position_x);
                if (dx >= 0) {
                    face = 0;
                } else {
                    face = 1;
                    dx = -dx;
                }
                if (ACTOR_M8(actor, facing_right) == face) {
                    ACTOR_M32(actor, state_flags) |= 0x110000;
                    ACTOR_M32(actor, behavior_phase) = 3;
                    ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                    ACTOR_M16(actor, animation_requested) = g_0608dae8[1];
                } else if (dx >= 0x80000) {
                    /* too far on X; stay */
                } else {
                    s32 dy = M32(tgt, 0x10) - ACTOR_M32(actor, position_z);
                    if (dy < 0) {
                        dy = -dy;
                    }
                    if (dy >= 0x8000) {
                        /* too far on Y; stay */
                    } else if ((u32)(g_frame_tick_legacy - ACTOR_M32(actor, action_start_tick)) <= 0x3CU) {
                        /* reacted too recently; stay */
                    } else if (collision_probe_between_actors(actor, tgt) != 0) {
                        /* helper vetoed; stay */
                    } else {
                        s32 bit;
                        ACTOR_M32(actor, state_flags) |= 0x100000;
                        ACTOR_M32(actor, behavior_phase) = 3;
                        ACTOR_M8(actor, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                        if (ACTOR_M8(actor, facing_right) == 0) {
                            face ^= 1;
                        }
                        bit = face & 1;
                        ACTOR_M16(actor, animation_requested) = g_0608dae8[bit];
                        if (bit != 0) {
                            ACTOR_M32(actor, state_flags) |= 0x10000;
                        }
                    }
                }
            }
        }
        break;
    }
    }
    return actor_anim_frame_update(ctx, actor);
}

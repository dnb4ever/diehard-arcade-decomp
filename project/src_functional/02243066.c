/* enemy_tick_grab_throw [0x02243066]: Thrower state machine uses controller table, anchor and interrupted-release path. */
#include "dh_work.h"
#include "dh.h"
/* 0x02243066  grab/throw actor state machine (the thrower side). arg0 = task,
 * inner = MP(task,8) is the actor; owner = g_0609196c[inner->unkE4] is this
 * actor's controller record (owner->unk8 = anchor x, owner->unk20 = next handler).
 *
 * Interrupt path: if func_02245dcc(inner) fires (grab broken/forced release),
 * snap to the thrown pose (unkD4=0x11, unkBA=0x1B), rebuild via func_0223ffcc,
 * heading 0x120, play 0xA01204FF, reset the matrix slot (func_02242fb0), install
 * owner->unk20 as the task handler and finalise (func_0222ca98).
 *
 * Otherwise step on M32(inner,0xD4):
 *   0 init: cache the grabbed victim (g_060973f8->unk8) in unk100, 5 struggle
 *     reps, face, advance.  1 reel-in: heading 0x47D, flag victim unk80|1, log a
 *     debris record, advance.  2 wind-up: aim victim angle, on may-act set heading
 *     0x47F, 3-frame hold, advance.  3 search: every 4th frame scan slots 1,2 for
 *     a counter-grab (func_02242fde) -> state 7; else tick the hold, release
 *     (func_02242f9c) to state 5.  5 recover-aim: ease victim angle; on may-act
 *     pick a random extra hold (func_02210020) and go to 6.  6 cooldown: tick,
 *     then either loop (unkD4=1) or finish: reload script g_0608db24, set the
 *     global throw bit g_06098580|4 and install handler func_0224680c.
 *   7/8 counter-grabbed: resolve via func_02245eb8 / func_02242f9c.
 * Always: animation_update, func_0222ca98, spatial_grid_insert tail.
 *
 * bounds: [0x243066, 0x2433e8); the 0x448 fn.extent over-swallows the draw
 *   function at 0x2433e8 (split at the braf-tail to func_0222eee4 at 0x2433e4).
 * notes: func_02210020 and func_02228106 take no args; func_02245eb8 and
 *   func_02242f9c take only the actor (extra call-site regs are dead).
 * fidelity: HIGH - field widths, masks and the slot scan verified vs fn.py --asm. */
extern void *g_0609196c[];     /* per-actor controller records */
extern void *g_060973f8;       /* current grab victim holder */
extern s32  g_sound_effects_enabled;        /* sound-enable gate */
extern s32  g_scene_event_flags;        /* global throw/live bitmask */
extern u8   g_06098838[];      /* debris records, 8-byte stride */
extern s32  g_frame_tick_legacy;        /* frame counter */
extern s32  g_actor_slot_ids[];      /* per-slot active flag */
extern void *g_actor_slot_tasks_legacy[];     /* per-slot anchor pointers; deref +8 -> actor */
extern s16  g_0608db24[];      /* recover script */

s32  enemy_attack_is_armed(void *actor);                 /* 0x02245dcc */
void actor_face_from_yaw_sign(void *actor);                 /* 0x02245e80 */
s32  actor_dispatch_action_and_finish(void *task, void *actor);     /* 0x0223ffcc */
void sound_queue_push(s32 cmd);                     /* 0x02215f6c sound_queue_push */
void prop_release_to_panel_handler(void);                        /* 0x02242fb0 */
void draw_actor_hier_shadowed_entry(void *actor);                 /* 0x0222ca98 shared tail */
s32  game_random_next(void);                        /* 0x02210020 rng */
void actor_script_begin(void *actor, void *cfg);      /* 0x02244aa8 actor_script_begin */
s32  enemy_test_player_pair_lock(void *actor);                 /* 0x02245eb8 */
void actor_request_animation47e(void *actor);                 /* 0x02242f9c */
s32  enemy_can_lunge_target(void *actor, void *other);    /* 0x02242fde */
void enemy_approach_entry_task(void *task);                  /* 0x0224680c next handler */
s32  actor_anim_frame_update(void *task, void *actor);     /* 0x0222dc2c animation_update */
void collision_insert_grid(void *actor);                 /* 0x0222eee4 spatial_grid_insert */

void enemy_tick_grab_throw(void *task) {
    void *inner = TASK_PTR(task, work);
    void *owner = g_0609196c[M32(inner, 0xE4)];
    s16 heading;

    if (enemy_attack_is_armed(inner) != 0) {
        actor_face_from_yaw_sign(inner);
        M32(inner, 0xBC) = M32(owner, 0x08);
        M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_REACTION;
        M16(inner, 0xBA) = 0x1B;
        M32(inner, 0x108) = 0;
        M32(inner, 0xD8) = actor_dispatch_action_and_finish(task, inner);
        M16(inner, 0x5C) = 0x120;
        if (g_sound_effects_enabled != 0) {
            sound_queue_push(0xA01204FF);
        }
        prop_release_to_panel_handler();
        TASK_PTR(task, handler) = MP(owner, 0x20);
        draw_actor_hier_shadowed_entry(inner);
        return;
    }

    heading = M16(inner, 0x58);

    switch (M32(inner, 0xD4)) {
    case 5: {
        void *victim = MP(inner, 0x100);
        if (!(((u16)MU16(inner, 0x54)) & 2)) {
            s32 a = -(s32)(u16)heading + 0xC;
            if (a < 0) {
                a = 0;
            }
            M16(victim, 0x26) = (s16)(0xC000 - (u16)a * 0x12FU);
        } else {
            M8(inner, 0x41) = 1;
            M16(inner, 0x5C) = M16(inner, 0x148);
            M32(inner, 0xC8) = (game_random_next() & 0xF) + 0x10;
            M16(victim, 0x26) = 0xC000;
            M32(victim, 0x80) &= ~1;
            M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_SCRIPTED_RESPONSE;
        }
        break;
    }
    case 6: {
        s32 t = M32(inner, 0xC8) - 1;
        M32(inner, 0xC8) = t;
        if ((t + 1) < 0) {
            s32 r = M32(inner, 0xC4) - 1;
            M32(inner, 0xC4) = r;
            if (r > 0) {
                M32(inner, 0xD4) = 1;
            } else {
                actor_script_begin(inner, g_0608db24);
                M32(inner, 0xBC) = M32(owner, 0x08);
                M32(inner, 0xD4) = 0;
                prop_release_to_panel_handler();
                g_scene_event_flags |= 4;
                TASK_PTR(task, handler) = (void *)enemy_approach_entry_task;
            }
        }
        break;
    }
    case 7:
        if (enemy_test_player_pair_lock(inner) != 0) {
            M32(inner, 0xE0) |= 0x800000;
        }
        if (((u16)MU16(inner, 0x54)) & 2) {
            if (M32(inner, 0xE0) & 0x800000) {
                M32(inner, 0xE0) &= 0xFF7FFFFF;
                M8(inner, 0x41) = 1;
                M16(inner, 0x5C) = 0x481;
                M32(inner, 0xD4) = 8;
            } else {
                actor_request_animation47e(inner);
                M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_RECOVERY;
            }
        }
        break;
    case 8:
        if (((u16)MU16(inner, 0x54)) & 2) {
            actor_request_animation47e(inner);
            M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_RECOVERY;
        }
        break;
    case 0:
        MP(inner, 0x100) = MP(g_060973f8, 8);
        M32(inner, 0xC4) = 5;
        M8(inner, 0x44) = 1;
        M32(inner, 0xD4) += 1;
        /* fallthrough */
    case 1: {
        void *victim = MP(inner, 0x100);
        M8(inner, 0x41) = 1;
        M16(inner, 0x5C) = 0x47D;
        M32(victim, 0x80) |= 1;
        if (M8(inner, 0x34) < 8) {
            u8 *rec = g_06098838 + M8(inner, 0x34) * 8;
            M32(rec, 0) = 5;
            M32(rec, 4) = 0x12C;
        }
        M32(inner, 0xD4) += 1;
        break;
    }
    case 2:
        M16(MP(inner, 0x100), 0x26) = (s16)(-(s32)((u16)heading * 0x12FU) + 0xC000);
        if (((u16)MU16(inner, 0x54)) & 2) {
            M8(inner, 0x41) = 1;
            M16(inner, 0x5C) = 0x47F;
            M32(inner, 0xC8) = 3;
            M32(inner, 0xBC) = M32(owner, 0x08);
            M32(inner, 0xD4) += 1;
        }
        break;
    case 3:
        if (!(g_frame_tick_legacy & 3)) {
            s32 k;
            for (k = 1; k <= 2; k++) {
                if (g_actor_slot_ids[k] != 0 &&
                    enemy_can_lunge_target(inner, TASK_PTR(g_actor_slot_tasks_legacy[k], work)) != 0) {
                    M8(inner, 0x41) = 1;
                    M16(inner, 0x5C) = 0x480;
                    M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_TURN;
                    break;
                }
            }
        }
        if (((u16)MU16(inner, 0x54)) & 2) {
            s32 t = M32(inner, 0xC8) - 1;
            M32(inner, 0xC8) = t;
            if (t <= 0) {
                actor_request_animation47e(inner);
                M32(inner, 0xD4) = DH_ENEMY_BEHAVIOR_RECOVERY;
            }
        }
        break;
    }

    actor_anim_frame_update(task, inner);
    draw_actor_hier_shadowed_entry(inner);
    collision_insert_grid(inner);
}

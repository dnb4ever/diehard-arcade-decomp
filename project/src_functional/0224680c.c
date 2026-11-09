/* enemy_approach_entry_task [0x0224680c]: Runs the offscreen/active entry phases, tests playfield bounds and hands control back to the type handler. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224680c  spawned-actor "approach / enter screen" handler. arg0 = task,
 * actor = MP(task,8), type = g_0609196c[actor->unkE4], box = per-actor collision
 * box g_060b6d9c[actor->unk34] (0x48 stride; +0x18/+0x20 = half extents).
 * State on M32(actor,0xD4):
 *   0 off-screen: if actor_script_step6 says despawn, hand back to the type handler
 *     (type->unk20), finalise (func_02245e80), park (unkBC=unk140, unk84=1, state
 *     0); else once the actor is both unobstructed (dispatch_cmd) and fully inside
 *     the active play area (g_060972d0..dc +/- box extents), go active (state 1).
 *   1 active: if func_02245dcc fires (forced release) revert to thrown
 *     (state 0x11, rebuild via func_0223ffcc); else if the early-out flag unk4C is
 *     set hand back and run func_022489d0; else if actor_script_step6 says despawn,
 *     hand back to state 0. Non-release paths step animation_update.
 * Both states finish via func_022462bc + spatial_grid_insert.
 *
 * bounds: [0x0224680c, 0x2469a0); split out of func_02246678's over-swallowed
 *   extent (braf-tail to func_0222eee4 at 0x24699c). The jsr-via-pool handlers
 *   resolve to func_02245e80 (mova 0x24684c + 0xFFFFF634) and animation_update
 *   (mova 0x246844 + 0xFFFE73E8). fidelity: HIGH - verified vs fn.py --asm. */
extern void *g_0609196c[];     /* enemy type records (->unk20 = type handler) */
extern u8   g_actor_movement_records_legacy[];      /* per-actor collision box, 0x48 stride */
extern s32  g_060972d0;        /* play area left */
extern s32  g_060972d4;        /* play area right */
extern s32  g_060972d8;        /* play area top */
extern s32  g_060972dc;        /* play area bottom */

s32  actor_script_step6(void *actor);                 /* 0x02244b1e despawn test */
void actor_face_from_yaw_sign(void *actor);                 /* 0x02245e80 finalise */
s32  enemy_attack_is_armed(void *actor);                 /* 0x02245dcc forced-release test */
s32  actor_dispatch_action_and_finish(void *task, void *actor);     /* 0x0223ffcc */
void actor_enter_hit_reaction(void *actor);                 /* 0x022489d0 */
void *dispatch_cmd(void *pos, void *box, s32 z, s32 idx); /* 0x02217838 hit-test */
s32  actor_anim_frame_update(void *task, void *actor);     /* 0x0222dc2c animation_update */
s32  actor_draw_by_model_mode(void *actor);                 /* 0x022462bc */
void collision_insert_grid(void *actor);                 /* 0x0222eee4 spatial_grid_insert */

void enemy_approach_entry_task(void *task) {
    void *actor = TASK_PTR(task, work);
    void *type = g_0609196c[ACTOR_M32(actor, state_index)];
    void *box = g_actor_movement_records_legacy + (s16)(ACTOR_M8(actor, slot_id) * 0x48);

    switch (ACTOR_M32(actor, behavior_state)) {
    case 0:
        if (actor_script_step6(actor) == 0) {
            TASK_PTR(task, handler) = MP(type, 0x20);
            actor_face_from_yaw_sign(actor);
            M32(actor, 0xBC) = M32(actor, 0x140);
            ACTOR_M32(actor, action_category) = 1;
            ACTOR_M32(actor, behavior_state) = 0;
        } else if (dispatch_cmd((u8 *)actor + 8, (u8 *)box + 0x18, 1, ACTOR_M8(actor, slot_id)) == 0 &&
                   ACTOR_M32(actor, position_x) >= (g_060972d0 + M32(box, 0x18)) &&
                   ACTOR_M32(actor, position_x) < (g_060972d4 - M32(box, 0x18)) &&
                   ACTOR_M32(actor, position_z) >= (g_060972d8 + M32(box, 0x20)) &&
                   ACTOR_M32(actor, position_z) < (g_060972dc - M32(box, 0x20))) {
            ACTOR_M32(actor, behavior_state) = 1;
            M32(actor, 0xBC) = M32(actor, 0x140);
            ACTOR_M32(actor, action_category) = 1;
        }
        actor_anim_frame_update(task, actor);
        break;
    case 1:
        if (enemy_attack_is_armed(actor) != 0) {
            actor_face_from_yaw_sign(actor);
            TASK_PTR(task, handler) = MP(type, 0x20);
            ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
            ACTOR_M32(actor, behavior_phase) = actor_dispatch_action_and_finish(task, actor);
        } else {
            if (ACTOR_M8(actor, hit_pending) != 0) {
                TASK_PTR(task, handler) = MP(type, 0x20);
                actor_face_from_yaw_sign(actor);
                actor_enter_hit_reaction(actor);
            } else if (actor_script_step6(actor) == 0) {
                TASK_PTR(task, handler) = MP(type, 0x20);
                ACTOR_M32(actor, behavior_state) = 0;
                ACTOR_M32(actor, action_category) = 1;
                actor_face_from_yaw_sign(actor);
            }
            actor_anim_frame_update(task, actor);
        }
        break;
    }

    actor_draw_by_model_mode(actor);
    collision_insert_grid(actor);
}

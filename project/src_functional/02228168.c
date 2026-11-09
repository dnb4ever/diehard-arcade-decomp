#include "dh_work.h"
#include "dh.h"
/* 0x0228168 (player_join_spawn) - build and arm the player actor for a late/second-player join
 * (arg0 = player index 0/1). Advances the join-state byte g_06098674[index], spawns the fixed-id
 * actor (slot index+1), locates its descriptor in the current scene's spawn list, stamps a
 * 44-byte-stride template (g_06060814[g_060B3D54]) plus per-player facing/position, HP and per-slot
 * bookkeeping, then runs the actor init sequence. Two entry states:
 *   g_06098674[index] == 1: BEGIN. Also seeds g_0609975C/g_06097420/g_0609978C per slot and tails
 *     animation_update(g_060995B0[unk34], entity).
 *   g_06098674[index] == 4: COMMIT. Uses the alternate per-slot seed, runs the extra
 *     func_0223dd96 when g_060E0E38[(unk34-1)*16]==1, and returns without the animation tail.
 *   (any other join-state: nothing).
 * The spawn pool context used is g_06099580 inside func_02227bc4; the join request object is
 * g_0609882C. Entry always pokes func_0220ac5a(scene->unk28).
 * fidelity: HIGH - all computed branches resolved (ptr_resolve/braf_resolve): func_02227bc4
 * (0x227bc4, r5=&func_02228508 is materialised but ignored by the fixed-id spawner), func_0220ac5a
 * (0x20ac5a), func_0223dcc2 (0x23dcc2), func_0223dd96 (0x23dd96), func_0222a91a (0x22a91a),
 * func_0222ca54 (0x22ca54), func_0222da90 (0x22da90), func_0221707c (0x21707c, 5 args, 5th on
 * stack), animation_update actor_anim_frame_update (0x22dc2c). mul.l is a 32-bit multiply (stride 44). unk34
 * byte, unk20/22/26/5E word, unk8/C/10/BC long, unk4E byte; g_06060814 44-byte records,
 * g_0609975C s32[], g_06097420/g_0609978C/g_06099794/g_06099796/g_0609979A s16, g_060995B0 ptr[],
 * g_060E0E38 16-byte records. */
extern void *actor_spawn_fixed_slot(s32 fixed_idx);
extern s32  resident_increment_counter104(s32 idx);
extern void player_clear_saved_action(s32 slot);
extern void player_restore_saved_action(s32 slot);
extern void actor_select_scene_variant(void *entity);
extern void actor_load_part_ids(void *obj);
extern void actor_load_current_pose_buffers(void *entity);
extern void actor_register_movement_slot(void *entity, s32 a, s32 b, s32 c, s32 d);
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update */
extern void *g_scene_descriptor;      /* -> scene descriptor */
extern u8   g_player_join_states[];     /* per-player join-state bytes */
extern void *g_view_task_legacy;      /* join request object */
extern s32  g_060B3D54;       /* template selector */
extern u8   g_06060814[];     /* 44-byte template records */
extern s32  g_play_session_mode;       /* two-player flag */
extern s32  g_view_request_priority;       /* progress */
extern s32  g_0609975C[];
extern s16  g_06097420[];
extern s16  g_0609978C[];
extern s16  g_06099796;
extern s16  g_06099794;
extern s16  g_actor_pose_set;
extern void *g_actor_slot_tasks[];
extern u8   g_060E0E38[];

/* copy the 44-byte template + per-player facing/position into the freshly spawned entity */
static void js_apply_template(void *entity, void *cursor) {
    s32 ti = 44 * g_060B3D54;
    M32(entity, 0x8)  = M32(g_06060814, ti);
    M32(entity, 0xC)  = M32(g_06060814, ti + 4);
    M32(entity, 0x10) = M32(g_06060814, ti + 8);
    if ((s8)M8(entity, ACTOR_SLOT_ID) == 1) {
        M16(entity, OBJ_ANGLE_Y) = (s16)0xC000;
        M32(entity, 0x8) = M32(entity, 0x8) - 0x10000;
        M8(entity, 0x44) = 1;
    } else {
        M16(entity, OBJ_ANGLE_Y) = 0x4000;
        M32(entity, 0x8) = M32(entity, 0x8) + 0x10000;
        M8(entity, 0x44) = 0;
    }
    M8(entity, 0x44) = (MU16(entity, OBJ_ANGLE_Y) == 0xC000) ? 1 : 0;
    M16(entity, 0x5E) = (s8)M8(cursor, 1);
    M32(entity, 0xBC) = -1;
    M8(entity, 0x4E) = 90;
    if (g_play_session_mode == 1 && g_view_request_priority >= 3) {
        void *req = TASK_PTR(g_view_task_legacy, work);
        g_view_request_priority = 3;
        M8(g_view_task_legacy, 0xD) = 1;
        M16(req, 0xAE) = 0;
        M16(req, 0xB0) = 0;
        M16(req, 0xB2) = 0;
    }
}

/* walk the scene spawn list to the descriptor whose unk0 matches this player slot */
static void *js_find_cursor(s32 slot) {
    void *cursor = MP(MP(g_scene_descriptor, 0x1C), 8);
    while ((s8)M8(cursor, 0) != slot)
        cursor = (u8 *)cursor + 24;
    return cursor;
}

void player_join_spawn(s32 index) {
    s32 slot = index + 1;

    resident_increment_counter104((s16)SCENE_M16(g_scene_descriptor, pose_index));

    if (g_player_join_states[index] == 1) {
        void *obj;
        void *entity;
        void *cursor;
        s32 n34;

        g_player_join_states[index] = 2;
        obj = actor_spawn_fixed_slot(index);
        if (obj == (void *)0)
            return;
        cursor = js_find_cursor(slot);
        entity = MP(obj, 8);
        js_apply_template(entity, cursor);
        n34 = (s8)M8(entity, ACTOR_SLOT_ID);
        g_0609975C[n34 - 1] = 30;
        g_06097420[n34 - 1] = 8;
        g_0609978C[n34 - 1] = g_06099796;
        M16(entity, 0x56) = (s16)(g_actor_pose_set == 1 ? (g_06099794 << 1) : g_06099794);
        player_clear_saved_action((s8)M8(entity, ACTOR_SLOT_ID));
        M16(entity, 0x20) = 0;
        M16(entity, 0x22) = 0;
        actor_select_scene_variant(entity);
        actor_load_part_ids(entity);
        actor_load_current_pose_buffers(entity);
        actor_register_movement_slot(entity, 0x5999, (s16)0xB334, 0x4000, 0x14CCC);
        actor_anim_frame_update(g_actor_slot_tasks[(s8)M8(entity, ACTOR_SLOT_ID)], entity);
    } else if (g_player_join_states[index] == 4) {
        void *obj;
        void *entity;
        void *cursor;
        s32 n34;

        g_player_join_states[index] = 2;
        obj = actor_spawn_fixed_slot(index);
        if (obj == (void *)0)
            return;
        cursor = js_find_cursor(slot);
        entity = MP(obj, 8);
        js_apply_template(entity, cursor);
        n34 = (s8)M8(entity, ACTOR_SLOT_ID);
        if (g_actor_pose_set == 1) {
            g_0609978C[n34 - 1] = 1;
            M16(entity, 0x56) = (s16)(g_06099794 << 1);
        } else {
            g_0609978C[n34 - 1] = g_06099796;
            M16(entity, 0x56) = g_06099794;
        }
        actor_select_scene_variant(entity);
        actor_load_part_ids(entity);
        if (g_060E0E38[(n34 - 1) * 16] == 1)
            player_restore_saved_action((s8)M8(entity, ACTOR_SLOT_ID));
        player_clear_saved_action((s8)M8(entity, ACTOR_SLOT_ID));
        M16(entity, 0x22) = 0;
        M16(entity, 0x20) = 0;
        actor_load_current_pose_buffers(entity);
        actor_register_movement_slot(entity, 0x5999, (s16)0xB334, 0x4000, 0x14CCC);
    }
}

/* enemy_initialize_spawn_task [0x02246678]: Loads enemy art, action and script descriptors and installs its per-type task handler. */
#include "dh_work.h"
#include "dh.h"
/* 0x02246678  spawned-actor initialiser (the handler func_022463b8 installs into
 * each freshly registered enemy). arg0 = task, actor = MP(task,8), type =
 * g_0609196c[actor->unkE4] is the enemy type record, art = type->unk1C its art
 * descriptor. Installs the real per-type handler g_060901b8[actor->unk104] into
 * task->unk0, resets combat state (func_0222da90/ca54/245c88), applies the palette
 * swap (func_0223cfe4 when unk45 set), loads the behaviour script (actor_script_begin
 * with unk118), builds the bounding box from the art record (func_0221707c), then
 * copies the type's flags/speed/reach (unk38/C0/140/FC/13C/146) into the actor,
 * sets hit cooldowns 0x7F, clears the per-actor scratch g_06097128[unk34], and for
 * flagged types seeds the shadow/marker slot g_060996ec[unk34] from g_060900e4 or
 * g_060900f0. Finally registers the actor's group bit in g_06098580.
 *
 * bounds: [0x02246678, 0x2467b4); the 0x440 fn.extent over-swallows func_0224680c,
 *   func_022469a0 and func_022469f2 (split at the braf-tail to func_0222eee4 at
 *   0x2467b0). fidelity: HIGH - widths from m2c casts / fn.py --asm. */
extern void *g_0609196c[];     /* enemy type records */
extern void *g_060901b8[];     /* per-type behaviour handlers (by unk104) */
extern u8   g_06097128[];      /* per-actor scratch, 0x24 stride */
extern u8   g_060996ec[];      /* per-actor shadow slot, 0xC stride */
extern u8   g_060900e4[];      /* default marker vector A */
extern u8   g_060900f0[];      /* default marker vector B */
extern s32  g_scene_event_flags;        /* group bitmask */

void actor_load_current_pose_buffers(void *actor);                 /* 0x0222da90 */
void actor_load_part_ids(void *actor);                 /* 0x0222ca54 */
s32  actor_begin_effect_action(void *actor, s32 action);     /* 0x0223cfe4 palette swap */
void enemy_load_action_descriptor(void *actor);                 /* 0x02245c88 */
void actor_script_begin(void *actor, void *cfg);      /* 0x02244aa8 actor_script_begin */
void actor_register_movement_slot(void *actor, s32 a1, s32 a2, s32 a3, s32 a4); /* 0x0221707c box */
s32  actor_anim_frame_update(void *task, void *actor);     /* 0x0222dc2c animation_update */
s32  actor_draw_by_model_mode(void *actor);                 /* 0x022462bc */
void collision_insert_grid(void *actor);                 /* 0x0222eee4 spatial_grid_insert */

void enemy_initialize_spawn_task(void *task) {
    void *actor = TASK_PTR(task, work);
    s8 idx = ACTOR_M8(actor, slot_id);
    void *type = g_0609196c[ACTOR_M32(actor, state_index)];
    void *art = MP(type, 0x1C);
    s8 pal;
    s32 group;

    TASK_PTR(task, handler) = g_060901b8[ACTOR_M32(actor, queued_attack_slot)];
    M16(actor, 0x22) = 0;
    actor_load_current_pose_buffers(actor);
    actor_load_part_ids(actor);

    pal = M8(actor, 0x45);
    if (pal != 0) {
        actor_begin_effect_action(actor, pal);
    }
    M8(actor, 0x45) = 0;
    M8(actor, 0x46) = 0;
    enemy_load_action_descriptor(actor);

    ACTOR_M32(actor, action_category) = (s32)0x80000200;
    ACTOR_M32(actor, action_state) = 0;
    actor_script_begin(actor, MP(actor, 0x118));
    actor_register_movement_slot(actor, M32(art, 0x00), M32(art, 0x04), M32(art, 0x08), M32(art, 0x0C));

    ACTOR_M32(actor, render_flags) = M32(type, 0x00);
    M32(actor, 0xC0) = M32(type, 0x0C);
    M32(actor, 0x140) = M32(type, 0x08);
    M32(actor, 0xBC) = -1;
    M32(actor, 0xFC) = M32(type, 0x10);
    M32(actor, 0x13C) = (M32(actor, 0xE8) << 0xA) + M32(type, 0x14);
    M16(actor, 0x146) = (s16)M32(type, 0x04);
    ACTOR_M32(actor, action_phase) = 0;
    M8(actor, 0x72) = 0x7F;
    M8(actor, 0x73) = 0x7F;
    M32(g_06097128 + (s16)(idx * 0x24), 0) = 0;

    if (ACTOR_M32(actor, render_flags) & 0x4100) {
        u8 *dst = g_060996ec + idx * 0xC;
        void *src = (ACTOR_M32(actor, render_flags) & 0x100) ? (void *)g_060900e4
                                               : (void *)g_060900f0;
        M32(dst, 0) = M32(src, 0);
        M32(dst, 4) = M32(src, 4);
        M32(dst, 8) = M32(src, 8);
    }

    group = ACTOR_M32(actor, queued_attack_kind);
    if (group != 0) {
        g_scene_event_flags |= ~group;
    }

    actor_anim_frame_update(task, actor);
    actor_draw_by_model_mode(actor);
    collision_insert_grid(actor);
}

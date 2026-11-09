/* actor_anim_step_task_source_alias [0x0222f4e8]: Existing map corroborated by source: per-frame path/anim actor task: record g_06097D58[unk34,unk5F]; partner-slot(unk5D) gated spawn of trail obj (spawn_obj_variant 0x220feec, handler 0x238d5e) seeded w (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0022f4e8  actor_anim_step_task
 *
 * Per-frame task handler for a path/animation-driven actor.
 *   arg0 = task ; entity = task->unk8 ; other = task->unk14->unk8.
 *
 * Each frame the current base position is read from the per-actor animation
 * record array g_06097D58, indexed by the actor's animation id (unk34, <<8)
 * and the current frame (unk5F, <<4). Field unk5D holds a "partner slot" that,
 * when set and a config bit is enabled, spawns a linked trail object whose
 * sub-struct (obj->unk8) gets this entity's position plus sin/cos velocities
 * derived from the partner record's angles (unk62, unk60). The partner link is
 * then propagated (actor_pair_propagate) and cleared.
 *
 * A movement sub-handler is dispatched from g_06091344[unk62 & 3]; a non-zero
 * return kills the task (task->unk0 = 0). Otherwise the previous position is
 * saved to unk4C/50/54, the new base position loaded, and the entity is added
 * to the draw list (tail call drawlist_prepend).
 *
 * Previously deferred: five computed branches. braf_resolve.py pins them:
 *   22f560 bsrf -> 0x220feec (spawn_obj_variant)
 *   22f578 bsrf -> 0x213190  (sin_lookup)
 *   22f58a bsrf -> 0x2131a8  (cos_lookup)
 *   22f59c bsrf -> 0x213190  (sin_lookup)
 *   22f5fc braf -> 0x22eea2  (drawlist_prepend, tail)
 *
 * fidelity: HIGH - all control flow, the config/flag gates, the spawn sub-block
 * field writes, the sin/cos shift amounts (>>4,>>4,>>3) and the dispatch/kill
 * path read straight from ROM asm; every computed branch target confirmed by
 * braf_resolve.py.
 */
extern s32   g_06098808;                 /* config write cursor */
extern u8    g_bios_status_buffer[];               /* config byte table */
extern void *g_actor_slot_tasks[];               /* per-slot record pointers */
extern s32 (*g_06091344[])(void *, void *); /* 4-entry movement sub-handler table */

extern void *task_spawn_child_work88(void *parent, void *handler);  /* spawn_obj_variant */
extern void  actor_publish_linked_slot_value(void *entity);                  /* actor_pair_propagate */
extern void  collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern s32   sin_lookup(s32 angle);                    /* sin_lookup */
extern s32   cos_lookup(s32 angle);                    /* cos_lookup */
extern void  effect_trail_fall_task(void *task);                    /* 0x02238d5e spawned trail handler */

#define G_RECORD_BASE 0x06097D58

void actor_anim_step_task_source_alias(void *task) {
    void *entity = TASK_PTR(task, work);
    void *other  = MP(TASK_PTR(task, parent), 8);
    void *record = (void *)(uintptr_t)(G_RECORD_BASE
                            + (M8(entity, 0x34) << 8)
                            + (MU8(entity, 0x5F) << 4));
    s32 slot = M8(entity, 0x5D);

    if (slot != 0) {
        if ((M16(entity, 0x62) & 0x80) && slot < 7
            && ((g_bios_status_buffer[g_06098808 + 1] >> 1) & 1)) {
            void *partner = TASK_PTR(g_actor_slot_tasks[slot], work);
            if (M32(partner, 0x108) != 6 && !(M32(partner, 0x38) & 0x400)) {
                void *obj = task_spawn_child_work88(task, (void *)effect_trail_fall_task);
                void *sub = MP(obj, 8);
                M32(sub, 0x08) = M32(entity, 0x08);
                M32(sub, 0x0C) = M32(entity, 0x0C);
                M32(sub, 0x10) = M32(entity, 0x10);
                M32(sub, 0x14) = sin_lookup(M16(partner, 0x62)) >> 4;
                M32(sub, 0x1C) = cos_lookup(M16(partner, 0x62)) >> 4;
                M32(sub, 0x18) = sin_lookup(M16(partner, 0x60)) >> 3;
            }
        }
        actor_publish_linked_slot_value(entity);
        M8(entity, 0x5D) = 0;
    }

    if (g_06091344[M16(entity, 0x62) & 3](entity, other) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(entity, 0x4C) = M32(entity, 0x08);
    M32(entity, 0x50) = M32(entity, 0x0C);
    M32(entity, 0x54) = M32(entity, 0x10);
    M32(entity, 0x08) = M32(record, 0x00);
    M32(entity, 0x0C) = M32(record, 0x04);
    M32(entity, 0x10) = M32(record, 0x08);
    collision_queue_actor(entity);
}

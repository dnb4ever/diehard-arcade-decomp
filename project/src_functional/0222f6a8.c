/* projectile_phase_track_task [0x0222f6a8]: Existing map corroborated by source: phase-driven sibling of actor_anim_step_task: same partner-spawn block (handler 0x238d5e); record offset by owner phase other.unk58 (<8:+0x90, ==8:set unk68 pickup + (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0222f6a8  projectile_phase_track_task
 *
 * Per-frame handler installed by func_0222f640; a phase-driven sibling of
 * func_0222f4e8.  arg0 = task; entity = task->unk8; other = task->unk14->unk8
 * (the owner task's actor, whose unk58 is the firing phase counter).
 *
 * When the partner slot unk5D is set and gated (unk62 bit7, slot<7, config bit,
 * partner alive) it spawns a trail object (func_0220feec, handler 0x238d5e),
 * seeds its sub-struct position and sin/cos velocities from the partner angles
 * (unk62>>4, unk62>>4, unk60>>3), then propagates and clears the slot.
 *
 * The animation record (g_06097d58, indexed by unk34) is selected by phase:
 *   phase < 8  -> record + 0x90
 *   phase == 8 -> set unk68 (anim<3: g_0609979a? g_0604ebe4[anim] : 6; else
 *                 g_0604ebe4[anim]) and use record + 0x70
 *   phase > 8  -> record + 0x70
 * Dispatch the first motion sub-handler g_06091344[0](entity, other); a non-zero
 * return kills the task. Otherwise save the previous position (unk4C/50/54), load
 * the new one from the record, and prepend to the draw list only when the phase
 * is outside [4,8].
 *
 * Previously deferred: five computed branches. braf_resolve.py pins them:
 *   22f706 -> 0x20feec (spawn_obj_variant)  22f71e -> 0x213190 (sin_lookup)
 *   22f730 -> 0x2131a8 (cos_lookup)         22f742 -> 0x213190 (sin_lookup)
 *   22f83c braf -> 0x22eea2 (drawlist_prepend, conditional tail)
 *
 * fidelity: HIGH - the spawn gate chain, phase-selected record offsets, the
 * unk68 pickup, the fixed-index dispatch and the [4,8] draw window all read from
 * ROM asm; every computed branch confirmed by braf_resolve.py.
 */
extern s32 (*g_06091344[])(void *, void *); /* motion sub-handler table */
extern void *g_actor_slot_tasks_legacy[];
extern s32 g_06098808;
extern u8  g_bios_status_buffer_legacy[];
extern s32 g_actor_slot_masks[];
extern s16 g_actor_pose_set_legacy;
extern u8  g_06097d58[];

extern void *task_spawn_child_work88(void *parent, void *handler);  /* spawn_obj_variant */
extern void  actor_publish_linked_slot_value(void *entity);                  /* actor_pair_propagate */
extern void  collision_queue_actor(void *entity);                 /* drawlist_prepend */
extern s32   sin_lookup(s32 angle);                    /* sin_lookup */
extern s32   cos_lookup(s32 angle);                    /* cos_lookup */
extern void  effect_trail_fall_task(void *task);                    /* 0x02238d5e spawned trail handler */

void projectile_phase_track_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *other  = MP(TASK_PTR(task, parent), 8);
    s32 slot = M8(entity, 0x5D);
    s32 phase;
    void *rec;

    if (slot != 0) {
        if ((M16(entity, 0x62) & 0x80) && slot < 7
            && ((g_bios_status_buffer_legacy[g_06098808 + 1] >> 1) & 1)) {
            void *partner = TASK_PTR(g_actor_slot_tasks_legacy[slot], work);
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

    phase = (u16)M16(other, 0x58);
    if (phase < 8) {
        rec = &g_06097d58[(s32)M8(entity, 0x34) * 0x100 + 0x90];
    } else {
        if (phase == 8) {
            if (M8(entity, 0x34) < 3)
                M32(entity, 0x68) = (g_actor_pose_set_legacy != 0)
                                    ? g_actor_slot_masks[M8(entity, 0x34)] : 6;
            else
                M32(entity, 0x68) = g_actor_slot_masks[M8(entity, 0x34)];
        }
        rec = &g_06097d58[(s32)M8(entity, 0x34) * 0x100 + 0x70];
    }

    if (g_06091344[0](entity, other) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(entity, 0x4C) = M32(entity, 0x08);
    M32(entity, 0x50) = M32(entity, 0x0C);
    M32(entity, 0x54) = M32(entity, 0x10);
    M32(entity, 0x08) = M32(rec, 0x00);
    M32(entity, 0x0C) = M32(rec, 0x04);
    M32(entity, 0x10) = M32(rec, 0x08);

    phase = (u16)M16(other, 0x58);
    if (phase < 4 || phase > 8)
        collision_queue_actor(entity);
}

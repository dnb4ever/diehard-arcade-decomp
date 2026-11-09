/* actor_tick_sway [0x02212b20]: Clamped sinusoidal velocity and periodically randomized phase; shared animation tail. */
#include "dh_work.h"
#include "dh.h"
/* 0x02212b20 (actor_update_sway) - entity-behaviour state machine for an actor that
 * sways with a clamped sinusoidal velocity and re-randomises periodically. No draw
 * stage of its own; frame advance is delegated to animation_update. Installed as a
 * task handler: task->unk8 = entity, task->unk10 = state byte.
 *
 * state 0 (init): snapshot rest position unk98 = unkC; zero phase/vel/accel
 *   (unk2A/2C/2E); unk34 = 6; set flag bit 0x80000000 in unk84; unk5E = 50;
 *   unk4F = 0; run setup func_0222ca54; seed unk5C from table[0] (g_0605355c) and
 *   call func_0222db44(entity); unk41 = 1; register an actor slot via
 *   actor_slot_record_reg(entity,0,0,0,0); advance to state 1 and fall through.
 * state 1 (update): if (unk54 & 2) reseed unk5C from table[rng & 7]; advance the
 *   animation; integrate unk2C += unk2E then unk2A += unk2C, clamp unk2C to
 *   [-256, 256]; bob unkC = unk98 + sin_lookup(unk2A); run draw_actor_hier_model; every
 *   32 frames (g_060987bc & 31 == 0) pick a new accel unk2E = (rng & 0x7F) - 64.
 * other states: no-op.
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf targets resolved;
 * clamp bounds, table stride, frame mask, s16 wraps exact. */

extern s32  game_random_next(void);                        /* rng_next */
extern void actor_load_part_ids(void *entity);                /* actor setup helper */
extern void actor_load_pose_buffers(void *entity);              /* actor init helper (r5 ignored) */
extern void actor_register_movement_slot(void *entity, s32 slot, s32 a1, s32 a2, s32 a3); /* slot reg */
extern void actor_anim_frame_update(void *task, void *entity);    /* animation_update */
extern s32  sin_lookup(s32 angle);                   /* sin_lookup */
extern void draw_actor_hier_model(void *entity);                /* post-move helper */

extern s16 g_0605355c[];    /* sway parameter table (s16) */
extern s32 g_frame_tick_legacy;      /* free-running frame counter */

void actor_tick_sway(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    if (state == 0) {
        M32(ent, 0x98) = M32(ent, 0xC);
        M16(ent, 0x2A) = 0;
        M16(ent, 0x2C) = 0;
        M16(ent, 0x2E) = 0;
        M8(ent, 0x34) = 6;
        MU32(ent, 0x84) |= 0x80000000u;
        M16(ent, 0x5E) = 50;
        M8(ent, 0x4F) = 0;
        actor_load_part_ids(ent);
        M16(ent, 0x5C) = g_0605355c[0];
        actor_load_pose_buffers(ent);
        M8(ent, 0x41) = 1;
        M16(ent, 0x5C) = g_0605355c[0];
        actor_register_movement_slot(ent, 0, 0, 0, 0);
        TASK_S8(task, state) = 1;
    } else if (state != 1) {
        return;
    }

    if ((u16)M16(ent, 0x54) & 2) {
        M16(ent, 0x5C) = g_0605355c[game_random_next() & 7];
    }
    actor_anim_frame_update(task, ent);

    M16(ent, 0x2C) += M16(ent, 0x2E);
    M16(ent, 0x2A) += M16(ent, 0x2C);
    if (M16(ent, 0x2C) < -256) {
        M16(ent, 0x2C) = (s16)0xFF00;
    } else if (M16(ent, 0x2C) > 256) {
        M16(ent, 0x2C) = 0x100;
    }

    M32(ent, 0xC) = M32(ent, 0x98) + sin_lookup(M16(ent, 0x2A));
    draw_actor_hier_model(ent);

    if ((g_frame_tick_legacy & 31) == 0) {
        M16(ent, 0x2E) = (s16)((game_random_next() & 0x7F) + 0xFFC0);
    }
}

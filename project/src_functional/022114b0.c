/* actor_tick_debris_emitter [0x022114b0]: Timed animation events scatter debris, fire burst and restart sequence. */
#include "dh_work.h"
#include "dh.h"
/* 0x022114b0 (actor_update_debris_emitter) - entity-behaviour state machine: a timed
 * emitter that scatters popup debris along an animation timeline, fires a one-shot
 * burst, then loops. Installed as a task handler: task->unk8 = entity, task->unk10 =
 * state. Draws each frame but does NOT register into the spatial grid.
 *
 * state 0 (init): zero unk58/5C/60/64/68; unk30 = 0x4DE; alloc collision slot
 *   (func_02228106 -> unk34, deactivate on failure); unk64 = rng & 15; -> state 1.
 * state 1: unk30 = 0x4DE; actor_slot_record_reg(entity,0,0,0,0); move func_022170ea(
 *   entity, 3); unk64 = (unk64 + 1) & 15; unk68 = 0; -> state 2.
 * state 2 (scatter): timeline = g_06090730[unk64]; entry = &timeline[unk68*8]; when the
 *   frame counter unk60 hits entry->unk0, spawn a popup (func_0220feec w/ func_0221185c)
 *   at entity pos + g_0605337c[(entry->unk4*12) & 0xFF] (3 s32 offsets), set child->unk58
 *   = entry->unk2, advance unk68. unk60++; at > 300 -> state 3.
 * state 3 (burst): register slot w/ g_06052254[0..3]; move; spawn a burst object
 *   (func_0220ff28 w/ func_0223194a) seeded child->unk3C=2, unk5F=0, unk60=0, unk62=28,
 *   unk64=0xA666, unk68=0x7FFFFF00, unk58=9; play SFX 0xA01249FF (gated by g_060E0DDC);
 *   -> state 4.
 * state 4 (loop/cleanup): unk60++; at > 480 reset to 0, play SFX 0xA0124AFF, -> state 1.
 *   If the shared slot g_06098670==1 and g_06098830>=9, latch g_06098830=9, mark
 *   g_0609882c->unk13=1 and clear its child unkAE/B0/B2. On odd frames cycle the spin
 *   frame (unk58, unk30 = g_0605333c[unk58]) and spawn an anim-seq child (func_0220feec
 *   w/ func_022119a4) copying the parent transform, with child->unk60 = (frame & 2) ?
 *   0 : 0x8000.
 *
 * Every state draws: unk5C += 0xFEC0; push matrix, mtx_load_abc, apply matrix to vec
 * @+8, rotate @+0x24, submit model, pop.
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf resolved; spawn handlers
 * resolved (ptr_resolve) to func_0221185c / func_0223194a / func_022119a4; no braf tail
 * (this family member does not grid-register). */

extern s32   actor_allocate_effect_slot(void);
extern s32   game_random_next(void);
extern void  actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3);
extern void  actor_resolve_movement(void *entity, s32 mode);
extern void *task_spawn_child_work88(void *task, void *handler);
extern void *task_spawn_list_work7c(void *task, void *handler);
extern s32   sound_queue_push(u32 sound_id);
extern void  actor_tick_spin_popup(void);
extern void  effect_follow_parent_task(void);
extern void  actor_tick_animation_sequence(void);
extern void  mtx_load_abc(void);
extern void  mtx_apply_vec_at8(void *entity);
extern void  mtx_rotate_zyx_at24(void *entity);
extern void  model_submit_culled(void *entity, s32 flag);

extern void *g_06090730[];   /* debris timeline pointer table (index = unk64) */
extern u8    g_0605337c[];   /* per-entry position offsets (12-byte records) */
extern s16   g_0605333c[];   /* spin-frame table */
extern s32   g_06052254[];   /* actor-slot parameter block */
extern s32   g_sound_effects_enabled;
extern s32   g_play_session_mode;
extern s32   g_view_request_priority;
extern void *g_view_task;
extern s32   g_frame_tick_legacy;
extern s32   g_matrix_stack_cursor;

void actor_tick_debris_emitter(void *task)
{
    void *ent = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        s8 slot;
        M32(ent, 0x58) = 0;
        M32(ent, 0x5C) = 0;
        M32(ent, 0x60) = 0;
        M32(ent, 0x64) = 0;
        M32(ent, 0x68) = 0;
        M16(ent, 0x30) = 0x4DE;
        slot = (s8)actor_allocate_effect_slot();
        M8(ent, 0x34) = slot;
        if (slot < 0) {
            TASK_PTR(task, handler) = 0;
            return;
        }
        M32(ent, 0x64) = game_random_next() & 15;
        TASK_S8(task, state) = 1;
    }
    /* fallthrough */
    case 1:
        M16(ent, 0x30) = 0x4DE;
        actor_register_movement_slot(ent, 0, 0, 0, 0);
        actor_resolve_movement(ent, 3);
        M32(ent, 0x64) = (M32(ent, 0x64) + 1) & 15;
        M32(ent, 0x68) = 0;
        TASK_S8(task, state) = 2;
    /* fallthrough */
    case 2: {
        s16 *entry = (s16 *)((u8 *)g_06090730[M32(ent, 0x64)] + M32(ent, 0x68) * 8);
        if (M32(ent, 0x60) == entry[0]) {
            void *child = MP(task_spawn_child_work88(task, (void *)&actor_tick_spin_popup), 8);
            u8 *rec = &g_0605337c[(u8)(entry[2] * 12)];
            M32(child, 0x8)  = M32(ent, 0x8)  + M32(rec, 0);
            M32(child, 0xC)  = M32(ent, 0xC)  + M32(rec, 4);
            M32(child, 0x10) = M32(ent, 0x10) + M32(rec, 8);
            M32(child, 0x58) = entry[1];
            M32(ent, 0x68) += 1;
        }
        M32(ent, 0x60) += 1;
        if (M32(ent, 0x60) > 300) {
            TASK_S8(task, state) = 3;
        }
        break;
    }
    case 3: {
        void *child;
        actor_register_movement_slot(ent, g_06052254[0], g_06052254[1], g_06052254[2], g_06052254[3]);
        actor_resolve_movement(ent, 3);
        child = MP(task_spawn_list_work7c(task, (void *)&effect_follow_parent_task), 8);
        M32(child, 0x3C) = 2;
        M8(child, 0x5F) = 0;
        M16(child, 0x60) = 0;
        M16(child, 0x62) = 28;
        M32(child, 0x64) = 0xA666;
        M32(child, 0x68) = 0x7FFFFF00;
        M16(child, 0x58) = 9;
        if (g_sound_effects_enabled != 0) {
            sound_queue_push(0xA01249FFu);
        }
        TASK_S8(task, state) = 4;
    }
    /* fallthrough */
    case 4:
        M32(ent, 0x60) += 1;
        if (M32(ent, 0x60) > 480) {
            TASK_S8(task, state) = 1;
            M32(ent, 0x60) = 0;
            if (g_sound_effects_enabled != 0) {
                sound_queue_push(0xA0124AFFu);
            }
        }
        if (g_play_session_mode == 1 && g_view_request_priority >= 9) {
            void *c = TASK_PTR(g_view_task, work);
            g_view_request_priority = 9;
            M8(g_view_task, 0x13) = 1;
            M16(c, 0xAE) = 0;
            M16(c, 0xB0) = 0;
            M16(c, 0xB2) = 0;
        }
        if (g_frame_tick_legacy & 1) {
            M32(ent, 0x58) = (M32(ent, 0x58) + 1) & 3;
            M16(ent, 0x30) = g_0605333c[M32(ent, 0x58)];
        }
        if (g_frame_tick_legacy & 1) {
            void *child = MP(task_spawn_child_work88(task, (void *)&actor_tick_animation_sequence), 8);
            M32(child, 0x8)  = M32(ent, 0x8);
            M32(child, 0xC)  = M32(ent, 0xC);
            M32(child, 0x10) = M32(ent, 0x10);
            M16(child, 0x24) = M16(ent, 0x24);
            M16(child, 0x26) = M16(ent, 0x26);
            M16(child, 0x28) = M16(ent, 0x28);
            M32(child, 0x4C) = M32(ent, 0x8);
            M32(child, 0x50) = M32(ent, 0xC);
            M32(child, 0x54) = M32(ent, 0x10);
            M32(child, 0x60) = (g_frame_tick_legacy & 2) ? 0 : 0x8000;
        }
        break;
    default:
        break;
    }

    M32(ent, 0x5C) += 0xFEC0;
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
}

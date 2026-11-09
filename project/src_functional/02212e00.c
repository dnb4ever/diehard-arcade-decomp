/* actor_tick_rising_effect [0x02212e00]: Armed trigger starts spinning rising effect, then blinking removal. */
#include "dh_work.h"
#include "dh.h"
/* 0x02212e00 (actor_update_risingfx) - entity-behaviour state machine for a rising,
 * spinning effect actor that eventually blinks out and despawns. Installed as a task
 * handler: task->unk8 = entity, task->unk10 = state. Survives via the shared draw/
 * grid-register tail (func_0222eee4).
 *
 * state 0 (init): unk2A = 0xC200; unk2C = 0x5000; unk2E = 0; unkBC = 0x7FFFFF00;
 *   unk38 = 0x80000; alloc collision slot (func_02228106) -> unk34; unk48 = 0;
 *   unkA8 = 0x16666; register slot with param block g_06052294[0..3]; -> state 1.
 * state 1 (armed): when unk48 != 0 and unk4A < 8, play SFX 0xA01232FF (gated by
 *   g_060E0DDC) and -> state 2.
 * state 2 (spawn): if func_0223e4a0() reports a slot, spawn func_0223cbb0(5,3,0);
 *   on success set unk74 = 0x1000, unk62 = 0xA000, unk60 = 0x2800 and hand to
 *   func_0223a924(obj->unk8->unk34, 3, &entity->unk8, entity). unk98 = 40; -> state 3.
 * state 3 (rise): unk24 += 0x400; unk28 += 0x600; unk9C += 0x1800; unk8 += 0x1200;
 *   unkC = (sin_lookup(unk9C) >> 3) + 0x2000; count unk98 down, and at 0 reset it to
 *   20 and -> state 4.
 * state 4 (fade): count unk98 down; at 0 release g_060996D4[unk34-8] (if unk34 > 8)
 *   and deactivate (task->unk0 = 0, no draw); otherwise blink - skip the draw on odd
 *   frames (g_060987bc & 1).
 *
 * Draw (all surviving states): push matrix; for state <= 2 also emit the base model
 * (func_02213ca4 transform of unk8/unk10, rotate @+0x2A, unk30 = 54); then emit the
 * detail model (apply matrix to &unk8, rotate @+0x24, unk30 = 5); pop; tail-call
 * func_0222eee4(entity).
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf/braf resolved;
 * func_0223e4a0 nullary, func_0223a924 4-arg; draw gate re-reads task->unk10. */

extern s32  actor_allocate_effect_slot(void);
extern void actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3);
extern s32  sound_queue_push(u32 sound_id);
extern s32  effect_pool_make_room(void);
extern void *effect_pool_spawn(s32 a0, s32 a1, void *a2);
extern void effect_pool_update_pose(s32 idx, s32 mode, void *src, void *save);
extern s32  sin_lookup(s32 angle);                 /* sin_lookup */
extern void mtx_load_abc(void);                      /* mtx_load_abc */
extern void matrix_translate_current(s32 x, s32 y, s32 z);       /* vec3_array_matmul */
extern void mtx_apply_vec_at0(void *vec);                 /* apply matrix to vec */
extern void mtx_rotate_zyx(void *angles);              /* mtx_rotate_zyx */
extern void model_submit_culled(void *entity, s32 flag);    /* submit_model_culled */
extern void collision_insert_grid(void *entity);              /* spatial_grid_insert */

extern s32 g_06052294[];
extern s32 g_sound_effects_enabled;
extern u8  g_effect_slot_occupied_legacy[];
extern s32 g_frame_tick_legacy;
extern s32 g_matrix_stack_cursor;

void actor_tick_rising_effect(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    switch (state) {
    case 0:
        M16(ent, 0x2A) = (s16)0xC200;
        M16(ent, 0x2C) = 0x5000;
        M16(ent, 0x2E) = 0;
        M32(ent, 0xBC) = 0x7FFFFF00;
        M32(ent, 0x38) = 0x80000;
        M8(ent, 0x34) = (s8)actor_allocate_effect_slot();
        M8(ent, 0x48) = 0;
        M32(ent, 0xA8) = 0x16666;
        actor_register_movement_slot(ent, g_06052294[0], g_06052294[1], g_06052294[2], g_06052294[3]);
        TASK_S8(task, state) = 1;
        break;
    case 1:
        if (M8(ent, 0x48) != 0 && M8(ent, 0x4A) < 8) {
            if (g_sound_effects_enabled != 0) {
                sound_queue_push(0xA01232FFu);
            }
            TASK_S8(task, state) = 2;
        }
        break;
    case 2:
        if (effect_pool_make_room() != 0) {
            void *obj = effect_pool_spawn(5, 3, 0);
            if (obj != 0) {
                M32(ent, 0x74) = 0x1000;
                M16(ent, 0x62) = (s16)0xA000;
                M16(ent, 0x60) = 0x2800;
                effect_pool_update_pose((s8)M8(MP(obj, 0x8), 0x34), 3, (u8 *)ent + 8, ent);
            }
        }
        M32(ent, 0x98) = 40;
        TASK_S8(task, state) = 3;
        break;
    case 3:
        M16(ent, 0x24) += 0x400;
        M16(ent, 0x28) += 0x600;
        M32(ent, 0x9C) += 0x1800;
        M32(ent, 0x8) += 0x1200;
        M32(ent, 0xC) = (sin_lookup(M32(ent, 0x9C)) >> 3) + 0x2000;
        M32(ent, 0x98) -= 1;
        if (M32(ent, 0x98) <= 0) {
            M32(ent, 0x98) = 20;
            TASK_S8(task, state) = 4;
        }
        break;
    case 4:
        M32(ent, 0x98) -= 1;
        if (M32(ent, 0x98) <= 0) {
            if (M8(ent, 0x34) > 8) {
                g_effect_slot_occupied_legacy[M8(ent, 0x34) - 8] = 0;
            }
            TASK_PTR(task, handler) = 0;
            return;
        }
        if (g_frame_tick_legacy & 1) {
            return;
        }
        break;
    default:
        break;
    }

    g_matrix_stack_cursor += 0x30;
    if (TASK_S8(task, state) <= 2) {
        mtx_load_abc();
        matrix_translate_current(M32(ent, 0x8), 0x13000, M32(ent, 0x10));
        mtx_rotate_zyx((u8 *)ent + 42);
        M16(ent, 0x30) = 54;
        model_submit_culled(ent, 0);
    }
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)ent + 8);
    mtx_rotate_zyx((u8 *)ent + 36);
    M16(ent, 0x30) = 5;
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
    collision_insert_grid(ent);
}

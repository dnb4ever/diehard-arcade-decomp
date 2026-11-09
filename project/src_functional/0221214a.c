/* actor_tick_orbit_spawner [0x0221214a]: Eight phases oscillate facing angle and spawn child on trigger. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221214a (actor_update_orbiter) - entity-behaviour state machine for an actor
 * that oscillates its facing angle in an 8-phase cycle, spawns a child on trigger,
 * then cleans up. Installed as a task handler: task->unk8 = entity, task->unk10 =
 * state. Survives via the shared draw/grid-register tail (func_0222eee4).
 *
 * state 0 (init): unkC += g_060522f4[1]; zero unk14/18/1C/78/7C/80; unk30 = 67;
 *   unkA8 = 0x7333; alloc collision slot (func_02228106) -> unk34, deactivating if
 *   it fails (slot < 0). On success: unk38 = 0x100000; unkBC = 0x7FFFFF00; register
 *   slot with g_060522f4[0..3]; unk62 = 0; unkC4 = (unk26 & 7) << 5; -> state 1,
 *   then run the update.
 * state 1 (update): unkC4++; phase = (unkC4 >> 5) & 7; nudge facing unk26 by +0x100
 *   (phase 1/7) or -0x100 (phase 3/5). When unk48 is set, clear it and - if unk4A <
 *   8 - go to state 2, play SFX 0xA01232FF (gated by g_060E0DDC), and if a slot is
 *   free (func_0223e4a0) spawn func_0223cbb0(11,3,0); on success scatter the child
 *   angle unk62 += (rng & 0x1FFF) + 0xF000 and hand it to func_0223a924(
 *   obj->unk8->unk34, 3, &entity->unk8, entity).
 * state 2 (cleanup): release_slot_entry(entity); release g_060996D4[unk34-8] (if unk34 >
 *   8); deactivate (task->unk0 = 0); no draw.
 * other states: no-op, no draw.
 *
 * Draw (states 0/1): push matrix, mtx_load_abc, apply matrix to vec @+8, rotate
 * @+0x24, submit model, pop, tail-call func_0222eee4(entity).
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf/braf resolved;
 * func_0223e4a0 nullary, func_0223a924 4-arg; r10 = 0x100 double-duty (table +0x100
 * offset AND angle step) decoded via absolute addresses. */

extern s32  actor_allocate_effect_slot(void);
extern void actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3);
extern s32  sound_queue_push(u32 sound_id);
extern s32  effect_pool_make_room(void);
extern void *effect_pool_spawn(s32 a0, s32 a1, void *a2);
extern s32  game_random_next(void);                      /* rng_next */
extern void effect_pool_update_pose(s32 idx, s32 mode, void *src, void *save);
extern void release_slot_entry(void *entity);              /* cleanup helper */
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at8(void *entity);
extern void mtx_rotate_zyx_at24(void *entity);
extern void model_submit_culled(void *entity, s32 flag);
extern void collision_insert_grid(void *entity);

extern s32 g_060522f4[];    /* actor-slot parameter block (+0=p0 .. +0xC=p3) */
extern s32 g_sound_effects_enabled;
extern u8  g_effect_slot_occupied_legacy[];
extern s32 g_matrix_stack_cursor;

void actor_tick_orbit_spawner(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    if (state == 2) {
        release_slot_entry(ent);
        if (M8(ent, 0x34) > 8) {
            g_effect_slot_occupied_legacy[M8(ent, 0x34) - 8] = 0;
        }
        TASK_PTR(task, handler) = 0;
        return;
    }
    if (state != 0 && state != 1) {
        return;
    }

    if (state == 0) {
        s8 slot;
        M32(ent, 0xC) += g_060522f4[1];
        M32(ent, 0x14) = 0;
        M32(ent, 0x18) = 0;
        M32(ent, 0x1C) = 0;
        M32(ent, 0x80) = 0;
        M32(ent, 0x7C) = 0;
        M32(ent, 0x78) = 0;
        M16(ent, 0x30) = 67;
        M32(ent, 0xA8) = 0x7333;
        slot = (s8)actor_allocate_effect_slot();
        M8(ent, 0x34) = slot;
        if (slot < 0) {
            TASK_PTR(task, handler) = 0;
            return;
        }
        M32(ent, 0x38) = 0x100000;
        M32(ent, 0xBC) = 0x7FFFFF00;
        actor_register_movement_slot(ent, g_060522f4[0], g_060522f4[1], g_060522f4[2], g_060522f4[3]);
        M16(ent, 0x62) = 0;
        M32(ent, 0xC4) = ((u16)M16(ent, 0x26) & 7) << 5;
        TASK_S8(task, state) = 1;
    }

    M32(ent, 0xC4) += 1;
    {
        s32 phase = (M32(ent, 0xC4) >> 5) & 7;
        if (phase == 1 || phase == 7) {
            M16(ent, 0x26) += 0x100;
        } else if (phase == 3 || phase == 5) {
            M16(ent, 0x26) -= 0x100;
        }
    }

    if (M8(ent, 0x48) != 0) {
        M8(ent, 0x48) = 0;
        if (M8(ent, 0x4A) < 8) {
            TASK_S8(task, state) = 2;
            if (g_sound_effects_enabled != 0) {
                sound_queue_push(0xA01232FFu);
            }
            if (effect_pool_make_room() != 0) {
                void *obj = effect_pool_spawn(11, 3, 0);
                if (obj != 0) {
                    M16(ent, 0x62) += (s16)((game_random_next() & 0x1FFF) + 0xF000);
                    effect_pool_update_pose((s8)M8(MP(obj, 0x8), 0x34), 3, (u8 *)ent + 8, ent);
                }
            }
        }
    }

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
    collision_insert_grid(ent);
}

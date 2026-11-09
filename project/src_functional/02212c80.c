/* actor_tick_breakable [0x02212c80]: Collision trigger leads to debris spawn, draw/grid registration and removal. */
#include "dh_work.h"
#include "dh.h"
/* 0x02212c80 (actor_update_breakable) - entity-behaviour state machine for a
 * breakable/destructible actor: spawn/arm -> wait-for-trigger -> break & spawn
 * debris -> despawn. Installed as a task handler: task->unk8 = entity,
 * task->unk10 = state byte. Every surviving frame it draws and registers itself
 * into the spatial grid (tail-call func_0222eee4).
 *
 * state 0 (init): unk24 = 0; unk26 = unk28 = 0x4000; unk30 = 11; unkBC =
 *   0x7FFFFF00; unk38 = 0x80000; allocate a collision slot via func_02228106 ->
 *   unk34; unk48 = 0; unkA8 = 0x4CCC; register the actor slot with the parameter
 *   block g_06052284[0..3] via actor_slot_record_reg; advance to state 1; draw.
 * state 1 (armed): if unk48 != 0 and unk4A < 8, set unk98 = 30, play the break
 *   SFX 0xA01233FF when g_060E0DDC is set, and advance to state 2; draw.
 * state 2 (break): if func_0223e4a0() reports a free slot, spawn a debris object
 *   func_0223cbb0(6,3,0); on success set unk74(32b) = 0x1000, unk62 = 0xE000,
 *   unk60 = 0x2800 and hand it to func_0223a924(obj->unk8->unk34, 3, &entity->unk8,
 *   entity). Release the shared occupancy byte g_060996D4[unk34-8] (when unk34 > 8)
 *   and deactivate the task (task->unk0 = 0); no draw.
 * other states: draw only.
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf/braf targets
 * resolved; func_0223e4a0 is nullary, func_0223a924 is 4-arg (per their decomps);
 * immediates/offsets exact. */

extern s32  actor_allocate_effect_slot(void);                              /* collision-slot alloc */
extern void actor_register_movement_slot(void *entity, s32 a0, s32 a1, s32 a2, s32 a3); /* slot reg */
extern s32  sound_queue_push(u32 sound_id);                      /* sfx_trigger */
extern s32  effect_pool_make_room(void);                              /* free-slot query */
extern void *effect_pool_spawn(s32 a0, s32 a1, void *a2);         /* spawn object */
extern void effect_pool_update_pose(s32 idx, s32 mode, void *src, void *save);
extern void mtx_load_abc(void);                              /* mtx_load_abc */
extern void mtx_apply_vec_at0(void *vec);                         /* apply matrix to vec */
extern void mtx_rotate_zyx(void *angles);                      /* mtx_rotate_zyx */
extern void model_submit_culled(void *entity, s32 flag);            /* submit_model_culled */
extern void collision_insert_grid(void *entity);                      /* spatial_grid_insert */

extern s32 g_06052284[];    /* actor-slot parameter block (4 words) */
extern s32 g_sound_effects_enabled;      /* sfx-enable gate */
extern u8  g_effect_slot_occupied_legacy[];    /* shared slot-occupancy flags */
extern s32 g_matrix_stack_cursor;      /* active matrix-stack pointer */

void actor_tick_breakable(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    switch (state) {
    case 0:
        M16(ent, 0x24) = 0;
        M16(ent, 0x26) = 0x4000;
        M16(ent, 0x28) = 0x4000;
        M16(ent, 0x30) = 11;
        M32(ent, 0xBC) = 0x7FFFFF00;
        M32(ent, 0x38) = 0x80000;
        M8(ent, 0x34) = (s8)actor_allocate_effect_slot();
        M8(ent, 0x48) = 0;
        M32(ent, 0xA8) = 0x4CCC;
        actor_register_movement_slot(ent, g_06052284[0], g_06052284[1], g_06052284[2], g_06052284[3]);
        TASK_S8(task, state) = 1;
        break;
    case 1:
        if (M8(ent, 0x48) != 0 && M8(ent, 0x4A) < 8) {
            M32(ent, 0x98) = 30;
            if (g_sound_effects_enabled != 0) {
                sound_queue_push(0xA01233FFu);
            }
            TASK_S8(task, state) = 2;
        }
        break;
    case 2:
        if (effect_pool_make_room() != 0) {
            void *obj = effect_pool_spawn(6, 3, 0);
            if (obj != 0) {
                M32(ent, 0x74) = 0x1000;
                M16(ent, 0x62) = (s16)0xE000;
                M16(ent, 0x60) = 0x2800;
                effect_pool_update_pose((s8)M8(MP(obj, 0x8), 0x34), 3,
                              (u8 *)ent + 8, ent);
            }
        }
        if (M8(ent, 0x34) > 8) {
            g_effect_slot_occupied_legacy[M8(ent, 0x34) - 8] = 0;
        }
        TASK_PTR(task, handler) = 0;
        return;
    default:
        break;
    }

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)ent + 8);
    mtx_rotate_zyx((u8 *)ent + 36);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
    collision_insert_grid(ent);
}

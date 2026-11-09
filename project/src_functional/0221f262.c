/* evt_actor_spawn_respawn_b [0x0221f262]: Existing map corroborated by source: spawn; timer=spawnlist->unk4; respawn on expiry */
#include "dh_work.h"
#include "dh.h"
/* 0x0221f262  evt_actor_spawn_respawn_b - scripted-event actor step (task FSM). The
 * separate handler that followed the braf in the merged extent at 0x0221f22e /
 * 0x0221f240. arg0 = task; task->unk8 = entity; task->unk10 = state. Same idiom as
 * evt_actor_spawn_respawn (0x0221d4fc):
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            entity->unk18 = entity->unk30->unk4;   (seed timer from spawn-list)
 *            task->unk10++.
 *   state 1: if (--entity->unk18 == 0) { process_spawn_list(entity); task->unk10++; }
 * fidelity: HIGH. bsrf resolved: scene_value_select 0x22a878, process_spawn_list
 * 0x22a5d0 (twice). Matches m2c_fn.py --asm. */

extern s32  scene_select_player_parameter(void);              /* scene_value_select */
extern void process_spawn_list(void *entity); /* 0x22a5d0 */

void evt_actor_spawn_respawn_b(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        SEQUENCE_M32(entity, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list(entity);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MP(entity, spawn_cursor), 0x4);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) == 0) {
            process_spawn_list(entity);
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
}

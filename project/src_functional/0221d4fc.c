/* evt_actor_spawn_respawn [0x0221d4fc]: Existing map corroborated by source: actor step FSM: spawn, timer=spawnlist->unk4, re-run spawn on expiry */
#include "dh_work.h"
#include "dh.h"
/* 0x0221d4fc evt_actor_spawn_respawn - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            entity->unk18 = entity->unk30->unk4;   (load countdown from spawn-list)
 *            task->unk10++.
 *   state 1: if (--entity->unk18 == 0) { process_spawn_list(entity); task->unk10++; }
 * fidelity: HIGH - entity->unk30 holds the spawn-list pointer (scene_value_select
 * result); its unk4 field seeds the timer; state-1 re-runs process_spawn_list.
 * bsrf resolved (0x22a878 / 0x22a5d0). Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(void *entity);

void evt_actor_spawn_respawn(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        SEQUENCE_M32(entity, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list(entity);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MP(entity, spawn_cursor), 0x4);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1: {
        s32 t = SEQUENCE_M32(entity, main_ticks) - 1;
        SEQUENCE_M32(entity, main_ticks) = t;
        if (t == 0) {
            process_spawn_list(entity);
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
    }
}

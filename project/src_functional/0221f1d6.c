/* evt_actor_spawn_wait11 [0x0221f1d6]: Existing map corroborated by source: actor step FSM: spawn, wait 11f on entity->unk1C, flag 1 */
#include "dh_work.h"
#include "dh.h"
/* 0x0221f1d6 evt_actor_spawn_wait11 - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            task->unk10++.
 *   state 1: ++entity->unk1C;  if (entity->unk1C - 1 == 10) { g_06098580 |= 1;
 *            task->unk10++; }   (fires on the 11th frame)
 * fidelity: HIGH - counter is pre-incremented then compared (new-1 == 10, i.e. old
 * == 10). bsrf resolved (0x22a878 / 0x22a5d0). Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(void *entity);
extern s32  g_scene_event_flags;

void evt_actor_spawn_wait11(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1: {
        s32 t = M32(entity, 0x1C) + 1;
        M32(entity, 0x1C) = t;
        if ((t - 1) == 10) {
            g_scene_event_flags |= 1;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
    }
}

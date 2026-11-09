/* evt_actor_spawn_wait100 [0x0221ccf8]: Existing map corroborated by source: actor step FSM: spawn, wait 100f on entity->unk18, flag, end */
#include "dh_work.h"
#include "dh.h"
/* 0x0221ccf8 evt_actor_spawn_wait100 - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index; task->unk0 = step fp.
 *   state 0: entity->unk30 = scene_value_select();  process_spawn_list(entity);
 *            task->unk10 = 1.
 *   state 1: if (++entity->unk18 > 100) { g_06098580 |= 1; task->unk10++; }
 *            (entity->unk18 is written back unconditionally - delay slot.)
 *   state 2: task->unk0 = 0  (terminate the step fp).
 * fidelity: HIGH - two bsrf resolved to scene_value_select (0x22a878) /
 * process_spawn_list (0x22a5d0, called with entity=task->unk8); unk10 byte state,
 * unk18 long counter; g_06098580 event-flags word. Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);     /* scene_value_select -> spawn-list ptr */
extern void process_spawn_list(void *entity);
extern s32  g_scene_event_flags;              /* global event-flags word */

void evt_actor_spawn_wait100(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        SEQUENCE_M32(entity, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list(entity);
        TASK_S8(task, state) = 1;
        break;
    case 1: {
        s32 t = SEQUENCE_M32(entity, main_ticks) + 1;
        SEQUENCE_M32(entity, main_ticks) = t;
        if (t > 100) {
            g_scene_event_flags |= 1;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
    case 2:
        TASK_S32(task, handler) = 0;
        break;
    }
}

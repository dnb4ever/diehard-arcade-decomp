/* evt_actor_spawn_4state [0x0221ca1e]: Existing map corroborated by source: actor step FSM (4 states): spawn, flag-refresh, re-spawn, end */
#include "dh_work.h"
#include "dh.h"
/* 0x0221ca1e evt_actor_spawn_4state - scripted-event actor step (task FSM, 4 states).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            entity->unk18 = entity->unk30->unk4;  task->unk10++.
 *   state 1: if (entity->unk18 == 0) {
 *                (*g_0609882C)->unk8->unk3C = 0x10;  func_0222a708();  task->unk10++;
 *            }
 *   state 2: process_spawn_list(entity);  entity->unk18 = 0x1E;  task->unk10++.
 *   state 3: if (--entity->unk18 == 0) task->unk0 = 0.
 * fidelity: HIGH - bsrf resolved: scene_value_select (0x22a878), process_spawn_list
 * (0x22a5d0), func_0222a708 (0x22a708, flag-refresh, void). g_0609882C is a global
 * object pointer. Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(void *entity);
extern void actor_mark_slots_for_refresh(void);
extern void *g_view_task_legacy;

void evt_actor_spawn_4state(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        M32(entity, 0x18) = M32(MP(entity, 0x30), 0x4);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1:
        if (M32(entity, 0x18) == 0) {
            M32(TASK_PTR(g_view_task_legacy, work), 0x3C) = 0x10;
            actor_mark_slots_for_refresh();
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    case 2:
        process_spawn_list(entity);
        M32(entity, 0x18) = 0x1E;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 3: {
        s32 t = M32(entity, 0x18) - 1;
        M32(entity, 0x18) = t;
        if (t == 0) {
            TASK_S32(task, handler) = 0;
        }
        break;
    }
    }
}

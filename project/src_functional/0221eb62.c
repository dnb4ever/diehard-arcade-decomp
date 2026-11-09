/* evt_actor_spawn_notify [0x0221eb62]: Existing map corroborated by source: actor step FSM: spawn, func_022332d4(g_060751B0), flag 4, 0x34f timer, flag 2 */
#include "dh_work.h"
#include "dh.h"
/* 0x0221eb62 evt_actor_spawn_notify - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            func_022332d4(g_060751B0);
 *            entity->unk18 = 0x34;  g_06098580 |= 4;  task->unk10 = 1.
 *   state 1: old = entity->unk18;  entity->unk18 = old - 1;
 *            if (old == 0) g_06098580 |= 2;
 * fidelity: HIGH - state-1 tests the pre-decrement value (add #-1 / add #1 / tst in
 * asm). bsrf resolved: scene_value_select (0x22a878), process_spawn_list (0x22a5d0),
 * func_022332d4 (0x2332d4, data-ptr arg 0x060751B0). Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(void *entity);
extern void scene_spawn_effect_list(void *arg);
extern s32  g_scene_event_flags;
extern u8   g_060751B0[];

void evt_actor_spawn_notify(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        scene_spawn_effect_list(g_060751B0);
        M32(entity, 0x18) = 0x34;
        g_scene_event_flags |= 4;
        TASK_S8(task, state) = 1;
        break;
    case 1: {
        s32 old = M32(entity, 0x18);
        M32(entity, 0x18) = old - 1;
        if (old == 0) {
            g_scene_event_flags |= 2;
        }
        break;
    }
    }
}

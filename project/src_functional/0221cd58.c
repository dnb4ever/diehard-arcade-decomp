/* scene_tick_spawn_count20 [0x0221cd58]: Spawn and flag-1 initialization followed by twenty-frame count and continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221cd58 evt_actor_spawn_cnt20 - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = byte state index; task->unk0 = step fp.
 *   state 0: *(s32*)0x06098580 |= 1 (delay-slot store completes before the call);
 *            entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            task->unk10 = 1.
 *   state 1: t = ++entity->unk18 (written back unconditionally in delay slot);
 *            if (t > 19) { func_02216c88(17, 8); task->unk0 = 0; }
 *   default: nothing (fallthrough to return).
 * fidelity: HIGH - bsrf targets resolved: scene_value_select 0x22a878,
 * process_spawn_list 0x22a5d0, func_02216c88 0x216c88 (r4=17, r5=8). */

extern s32  scene_select_player_parameter(void);     /* scene_value_select */
extern void process_spawn_list(void *entity);
extern void stage_build_render_grid(s32 stage, s32 variant);
extern volatile s32 g_scene_event_flags;      /* event-flags word */

void scene_tick_spawn_count20(void *task) {
    void *entity = TASK_PTR(task, work);
    switch ((s8)TASK_S8(task, state)) {
    case 0: {
        s32 v;
        g_scene_event_flags = g_scene_event_flags | 1;
        v = scene_select_player_parameter();
        M32(entity, 0x30) = v;
        process_spawn_list(entity);
        TASK_S8(task, state) = 1;
        break;
    }
    case 1: {
        s32 t = M32(entity, 0x18) + 1;
        M32(entity, 0x18) = t;
        if (t > 19) {
            stage_build_render_grid(17, 8);
            TASK_S32(task, handler) = 0;
        }
        break;
    }
    default:
        break;
    }
}

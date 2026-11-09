/* evt_actor_spawn_sfx40 [0x0221cee8]: Existing map corroborated by source: actor step FSM: spawn, sfx A01208FF, 0x28f timer, sfx A01209FF, end */
#include "dh_work.h"
#include "dh.h"
/* 0x0221cee8 evt_actor_spawn_sfx40 - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            if (g_060E0DDC != 0) sfx_trigger(0xA01208FF);
 *            entity->unk1C = 0x28;  task->unk10++.
 *   state 1: if (--entity->unk1C == 0) {
 *                if (g_060E0DDC != 0) sfx_trigger(0xA01209FF);
 *                task->unk0 = 0;   (terminate)
 *            }
 * fidelity: HIGH - like 0x21ce6e but no g_06098580 set in state 0 and the state-1
 * tail clears the step fp instead of advancing. bsrf resolved as in sibling.
 * Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(void *entity);
extern void sound_queue_push(u32 id);
extern s32  g_sound_effects_enabled_legacy;

void evt_actor_spawn_sfx40(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        if (g_sound_effects_enabled_legacy != 0) {
            sound_queue_push(DH_SOUND_EVENT_SECOND_PAIR_BEGIN_COMMAND);
        }
        M32(entity, 0x1C) = 0x28;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1: {
        s32 t = M32(entity, 0x1C) - 1;
        M32(entity, 0x1C) = t;
        if (t == 0) {
            if (g_sound_effects_enabled_legacy != 0) {
                sound_queue_push(DH_SOUND_EVENT_SECOND_PAIR_END_COMMAND);
            }
            TASK_S32(task, handler) = 0;
        }
        break;
    }
    }
}

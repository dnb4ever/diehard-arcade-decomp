/* evt_actor_spawn_sfx66 [0x0221ce6e]: Existing map corroborated by source: actor step FSM: spawn+flag 0xC, sfx A01206FF, 0x42f timer, sfx A01207FF */
#include "dh_work.h"
#include "dh.h"
/* 0x0221ce6e evt_actor_spawn_sfx66 - scripted-event actor step (task FSM).
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            g_06098580 |= 0xC;
 *            if (g_060E0DDC != 0) sfx_trigger(0xA01206FF);
 *            entity->unk1C = 0x42;  task->unk10++.
 *   state 1: if (--entity->unk1C == 0) {
 *                if (g_060E0DDC != 0) sfx_trigger(0xA01207FF);
 *                task->unk10++;
 *            }
 * fidelity: HIGH - bsrf resolved: scene_value_select (0x22a878), process_spawn_list
 * (0x22a5d0, entity arg), sfx_trigger (0x215f6c, u32 id arg). unk1C long countdown;
 * g_060E0DDC gate flag. Matches m2c --asm. */
extern s32  scene_select_player_parameter(void);
extern void process_spawn_list(void *entity);
extern void sound_queue_push(u32 id);   /* sound-effect trigger */
extern s32  g_scene_event_flags;
extern s32  g_sound_effects_enabled_legacy;

void evt_actor_spawn_sfx66(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        g_scene_event_flags |= 0xC;
        if (g_sound_effects_enabled_legacy != 0) {
            sound_queue_push(DH_SOUND_EVENT_PAIR_BEGIN_COMMAND);
        }
        M32(entity, 0x1C) = 0x42;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1: {
        s32 t = M32(entity, 0x1C) - 1;
        M32(entity, 0x1C) = t;
        if (t == 0) {
            if (g_sound_effects_enabled_legacy != 0) {
                sound_queue_push(DH_SOUND_EVENT_PAIR_END_COMMAND);
            }
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }
    }
}

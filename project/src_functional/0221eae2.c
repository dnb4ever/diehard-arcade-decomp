/* evt_actor_spawn_notify_sfx [0x0221eae2]: Existing map corroborated by source: spawn; func_022332d4(g_060751B0); flag 0xC; sfx */
#include "dh_work.h"
#include "dh.h"
/* 0x0221eae2  evt_actor_spawn_notify_sfx - scripted-event actor step (task FSM). The
 * separate handler that followed the braf in the merged extent at 0x0221eaac /
 * 0x0221eac0. arg0 = task; task->unk8 = entity; task->unk10 = state. Sibling of
 * evt_actor_spawn_notify (0x0221eb62):
 *   state 0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *            func_022332d4(g_060751B0);
 *            if (g_sfxEnableFlag) sound_queue_push(0xA01207FF);
 *            entity->unk1C = 0x38;  task->unk10 = 1.
 *   state 1: if (--entity->unk1C == 0) {
 *               g_06098580 |= 0xC;
 *               if (g_sfxEnableFlag) sound_queue_push(0xA01208FF);
 *               task->unk10 = 2; }
 * fidelity: HIGH. bsrf resolved: scene_value_select 0x22a878, process_spawn_list
 * 0x22a5d0, func_022332d4 0x2332d4 (data-ptr arg 0x060751B0), sound_queue_push
 * 0x215f6c. Both sfx are gated on *0x060e0ddc. Matches m2c_fn.py --asm. */

extern s32  scene_select_player_parameter(void);              /* scene_value_select */
extern void process_spawn_list(void *entity); /* 0x22a5d0 */
extern void scene_spawn_effect_list(void *arg);         /* 0x2332d4 */
extern void sound_queue_push(s32 cmd);           /* 0x215f6c sound_queue_push */
extern s32  g_scene_event_flags;                       /* event-flags word */
extern s32  g_sound_effects_enabled;                       /* sfx-enable gate */
extern u8   g_060751B0[];

void evt_actor_spawn_notify_sfx(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        scene_spawn_effect_list(g_060751B0);
        if (g_sound_effects_enabled) {
            sound_queue_push(0xa01207ff);
        }
        M32(entity, 0x1C) = 0x38;
        TASK_S8(task, state) = 1;
        break;
    case 1:
        M32(entity, 0x1C) -= 1;
        if (M32(entity, 0x1C) == 0) {
            g_scene_event_flags |= 0xC;
            if (g_sound_effects_enabled) {
                sound_queue_push(0xa01208ff);
            }
            TASK_S8(task, state) = 2;
        }
        break;
    }
}

/* evt_actor_spawn_child [0x0221f004]: Existing map corroborated by source: one-shot: spawn, flag 0xC, func_0220fece(task,&func_0221ad06) child, attach g_06075244 */
#include "dh_work.h"
#include "dh.h"
/* 0x0221f004 evt_actor_spawn_child - scripted-event actor step; one-shot on state 0.
 * arg0 = task; task->unk8 = entity; task->unk10 = state index.
 *   if (task->unk10 == 0) {
 *       entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *       g_06098580 |= 0xC;
 *       child = func_0220fece(task, &func_0221ad06);   (spawn a child task/entity)
 *       child->unk8->unk64 = &g_06075244;               (attach config record)
 *       task->unk10++;
 *   }
 * fidelity: HIGH - bsrf resolved: scene_value_select (0x22a878), process_spawn_list
 * (0x22a5d0, entity), func_0220fece (0x20fece; (task, template) -> spawn holder; r4
 * from delay slot = task, r5 = &func_0221ad06 via mova+add -> 0x21ad06). States 1+ are
 * no-ops. Matches m2c --asm. */
extern s32   scene_select_player_parameter(void);
extern void  process_spawn_list(void *entity);
extern void *task_spawn_current_work6c(void *task, void *template);
extern void  sound_tick_timeline_cues(void);     /* spawn template handler */
extern s32   g_scene_event_flags;
extern u8    g_06075244[];

void evt_actor_spawn_child(void *task) {
    void *entity = TASK_PTR(task, work);

    if (TASK_S8(task, state) == 0) {
        void *child;
        M32(entity, 0x30) = scene_select_player_parameter();
        process_spawn_list(entity);
        g_scene_event_flags |= 0xC;
        child = task_spawn_current_work6c(task, (void *)sound_tick_timeline_cues);
        MP(MP(child, 0x8), 0x64) = g_06075244;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
}

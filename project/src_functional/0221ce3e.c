/* evt_actor_spawn_flag1_once [0x0221ce3e]: Existing map corroborated by source: one-shot: g_06098580|=1; pick+spawn list; terminate */
#include "dh_work.h"
#include "dh.h"
/* 0x0221ce3e  evt_actor_spawn_flag1_once - scripted-event actor step (one-shot task).
 * The separate handler that followed the braf in the merged extent at 0x0221ce1c.
 * arg0 = task; task->unk8 = entity.
 *   g_06098580 |= 1;                              (raise event-flag bit 0)
 *   entity->unk30 = scene_value_select();         (pick the active spawn-list ptr)
 *   process_spawn_list(entity);                   (walk + spawn it)
 *   task->unk0 = 0;                               (clear step fp -> terminate)
 * fidelity: HIGH. The flag OR is the delay-slot store of the scene_value_select bsrf
 * (completes before the call). bsrf resolved: scene_value_select 0x22a878,
 * process_spawn_list 0x22a5d0. Matches m2c_fn.py --asm. */

extern s32  scene_select_player_parameter(void);              /* scene_value_select */
extern void process_spawn_list(void *entity); /* 0x22a5d0 */
extern s32  g_scene_event_flags;                       /* event-flags word */

void evt_actor_spawn_flag1_once(void *task) {
    void *entity = TASK_PTR(task, work);
    g_scene_event_flags |= 1;
    M32(entity, 0x30) = scene_select_player_parameter();
    process_spawn_list(entity);
    TASK_PTR(task, handler) = 0;
}

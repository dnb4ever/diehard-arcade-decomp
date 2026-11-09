/* player_initialize_task [0x02228508]: Initializes actor substate, animation/draw and changes task handler to main update. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228508 (actor FSM step) - task handler (r4=task, entity=task->unk8). Runs the
 * per-frame sub-update func_02228536(entity), the shared animation_update(task,entity),
 * and func_0222ca98(entity), then advances the task's handler pointer (task->unk0) to
 * the next step func_0222863e.
 * fidelity: HIGH - entity = task->unk8 (pointer); task->unk0 is a 32-bit handler-pointer
 * store; the three calls + the pointer store taken directly from the asm. Computed
 * tail/call branches resolved via tools/decomp/braf_rewrite.py (bsrf 0x570e -> 0x0222dc2c,
 * bsrf 0x4574 -> 0x0222ca98). */
extern void actor_substate_init(void *entity);
extern s32  actor_anim_frame_update(void *task, void *entity);   /* animation_update */
extern void draw_actor_hier_shadowed_entry(void *entity);
extern void actor_update_task(void);

void player_initialize_task(void *task) {
    void *entity = TASK_PTR(task, work);
    actor_substate_init(entity);
    actor_anim_frame_update(task, entity);
    draw_actor_hier_shadowed_entry(entity);
    TASK_PTR(task, handler) = (void *)actor_update_task;
}

/* enemy_update_state_task [0x022469a0]: Handles forced reactions, dispatches behavior state and finishes drawing and grid insertion. */
#include "dh_work.h"
#include "dh.h"
/* 0x022469a0  spawned-actor state dispatcher. arg0 = task, actor = MP(task,8).
 * If func_02245dcc fires (forced release) switch to the thrown state 0x11 and
 * rebuild via func_0223ffcc. Then dispatch to the per-state sub-handler
 * g_060901c4[actor->unkD4](task, actor), and finish with func_022462bc +
 * spatial_grid_insert.
 *
 * bounds: [0x022469a0, 0x2469f2); split out of func_02246678's over-swallowed
 *   extent (braf-tail to func_0222eee4 at 0x2469ee; func_022469f2 follows).
 * fidelity: HIGH - verified vs fn.py --asm. */
extern void *g_060901c4[];     /* per-state sub-handlers (by unkD4) */

s32  enemy_attack_is_armed(void *actor);                 /* 0x02245dcc forced-release test */
s32  actor_dispatch_action_and_finish(void *task, void *actor);     /* 0x0223ffcc */
s32  actor_draw_by_model_mode(void *actor);                 /* 0x022462bc */
void collision_insert_grid(void *actor);                 /* 0x0222eee4 spatial_grid_insert */

void enemy_update_state_task(void *task) {
    void *actor = TASK_PTR(task, work);
    void (*handler)(void *, void *);

    if (enemy_attack_is_armed(actor) != 0) {
        ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
        ACTOR_M32(actor, behavior_phase) = actor_dispatch_action_and_finish(task, actor);
    }

    handler = (void (*)(void *, void *))g_060901c4[ACTOR_M32(actor, behavior_state)];
    handler(task, actor);

    actor_draw_by_model_mode(actor);
    collision_insert_grid(actor);
}

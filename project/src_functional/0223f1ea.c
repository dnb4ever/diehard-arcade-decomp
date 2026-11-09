/* enemy_update_category_d_task [0x0223f1ea]: Dispatches the fourth actor category's behavior state, draw and grid continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223f1ea - func_0223f1ea: task handler (task->8 = entity) that runs the
 * entity's per-state move handler then the shared post-step helpers.
 *   obj = *(task+8);
 *   if (actor_preattack_gate(obj)) { obj->0xD4 = 0x11; obj->0xD8 = func_0223ffcc(task); }
 *   g_0608d714[obj->0xD4](task, obj);     // per-state handler, fn-ptr table
 *   func_0222ca98(obj);
 *   func_0222eee4(obj);                   // tail call (braf)
 * fidelity: HIGH - all 4 bsrf/braf resolve cleanly. r14=obj=*(task+8); the
 * table index obj->0xD4 is read as s32 (mov.l); the stack spill of the fn ptr
 * is a scratch artifact and is omitted. The final braf is a true tail call;
 * modeled as a plain call since this handler returns void. */
typedef void (*state_handler)(void *task, void *obj);

extern s32  enemy_attack_is_armed(void *obj);          /* 0x02245dcc actor_preattack_gate */
extern s32  actor_dispatch_action_and_finish(void *task);         /* 0x0223ffcc */
extern void draw_actor_hier_shadowed_entry(void *obj);          /* 0x0222ca98 */
extern void collision_insert_grid(void *obj);          /* 0x0222eee4 */
extern state_handler g_0608d714[];             /* per-state handler table, idx obj->0xD4 */

void enemy_update_category_d_task(void *task)
{
    void *obj = TASK_PTR(task, work);

    if (enemy_attack_is_armed(obj) != 0) {
        M32(obj, 0xD4) = DH_ENEMY_BEHAVIOR_REACTION;
        M32(obj, 0xD8) = actor_dispatch_action_and_finish(task);
    }

    g_0608d714[M32(obj, 0xD4)](task, obj);
    draw_actor_hier_shadowed_entry(obj);
    collision_insert_grid(obj);
}

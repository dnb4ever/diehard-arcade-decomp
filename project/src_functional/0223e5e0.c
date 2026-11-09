/* enemy_update_category_a_task [0x0223e5e0]: Dispatches behavior states for the first actor category, then draws and inserts its collision grid node. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223e5e0 - func_0223e5e0: actor per-frame update dispatcher.
 *   ent = task->unk8. If func_02245dcc(ent) != 0 (state-change requested):
 *     ent->unkD4 = 0x11; ent->unkD8 = func_0223ffcc(task, ent).
 *   g_0608d624[ent->unkD4](task, ent); func_0222ca98(ent);
 *   tail-call (braf) func_0222eee4(ent).
 * The sibling func_0223e632 follows immediately in ROM (fn.py did not split them
 * because this one ends in a braf tail-call); it lives in 0223e632.c.
 * fidelity: HIGH - bsrf/braf targets resolved: 0x245dcc, 0x23ffcc, 0x22ca98,
 *   0x22eee4 (tail). */
extern void (*g_0608d624[])(void *task, void *ent);
extern s32  enemy_attack_is_armed(void *ent);
extern s32  actor_dispatch_action_and_finish(void *task, void *ent);
extern void draw_actor_hier_shadowed_entry(void *ent);
extern void collision_insert_grid(void *ent);

void enemy_update_category_a_task(void *task) {
    void *ent = TASK_PTR(task, work);
    if (enemy_attack_is_armed(ent) != 0) {
        M32(ent, 0xD4) = DH_ENEMY_BEHAVIOR_REACTION;
        M32(ent, 0xD8) = actor_dispatch_action_and_finish(task, ent);
    }
    g_0608d624[M32(ent, 0xD4)](task, ent);
    draw_actor_hier_shadowed_entry(ent);
    collision_insert_grid(ent);
}

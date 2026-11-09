/* enemy_update_category_c_task [0x0223f198]: Dispatches the third actor category's behavior state, draw and grid continuation. */
#include "dh_work.h"
#include "dh.h"
/* func_0223f198(task): actor state-handler driver (sibling of func_0223e878 with a
 * different callee set). entity = task->unk8. If func_02245dcc(entity) != 0, set
 * entity->unkD4 = 0x11 and unkD8 = func_022401d6(task, entity). Dispatch
 * g_0608D6C4[unkD4](task, entity), then func_0222ca98(entity), tail func_0222eee4(entity).
 * fidelity: HIGH - offsets 0x8/0xD4/0xD8 and 4-byte table g_0608D6C4 verified; computed
 *   calls resolved via braf. */
extern s32  enemy_attack_is_armed(void *ent);
extern s32  actor_dispatch_action_preserve_motion(void *task, void *ent);
extern void draw_actor_hier_shadowed_entry(void *ent);
extern void collision_insert_grid(void *ent);
extern u8   g_0608D6C4[];   /* fn-ptr table (stride 4) */

void enemy_update_category_c_task(void *task) {
    void *ent = TASK_PTR(task, work);
    if (enemy_attack_is_armed(ent) != 0) {
        M32(ent, 0xD4) = DH_ENEMY_BEHAVIOR_REACTION;
        M32(ent, 0xD8) = actor_dispatch_action_preserve_motion(task, ent);
    }
    ((void (*)(void *, void *))MP(g_0608D6C4, M32(ent, 0xD4) * 4))(task, ent);
    draw_actor_hier_shadowed_entry(ent);
    collision_insert_grid(ent);
}

/* enemy_update_skeletal_category_task [0x0223e878]: Dispatches the skeletal category's behavior states, then draws and inserts its grid node. */
#include "dh_work.h"
#include "dh.h"
/* func_0223e878(task): actor state-handler driver. entity = task->unk8.
 * If func_02245dcc(entity) is nonzero, set entity->unkD4 = 0x11 and
 * entity->unkD8 = func_02240054(task, entity). Dispatch g_0608D674[unkD4](task, entity),
 * then func_0222d450(entity), then tail func_0222eee4(entity).
 * fidelity: HIGH - offsets 0x8/0xD4/0xD8 and 4-byte fn-ptr table g_0608D674 verified;
 *   all computed calls resolved via braf. */
extern s32  enemy_attack_is_armed(void *ent);
extern s32  actor_dispatch_action_with_effects(void *task, void *ent);
extern void actor_draw_skeletal_mesh(void *ent);
extern void collision_insert_grid(void *ent);
extern u8   g_0608D674[];   /* fn-ptr table (stride 4) */

void enemy_update_skeletal_category_task(void *task) {
    void *ent = TASK_PTR(task, work);
    if (enemy_attack_is_armed(ent) != 0) {
        M32(ent, 0xD4) = DH_ENEMY_BEHAVIOR_REACTION;
        M32(ent, 0xD8) = actor_dispatch_action_with_effects(task, ent);
    }
    ((void (*)(void *, void *))MP(g_0608D674, M32(ent, 0xD4) * 4))(task, ent);
    actor_draw_skeletal_mesh(ent);
    collision_insert_grid(ent);
}

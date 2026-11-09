/* enemy_spawn_clash_task [0x022490a0]: Creates a two-actor clash controller, binds both actors and enters state ten/substate two. */
#include "dh_work.h"
#include "dh.h"
/* 0x022490a0  actor_spawn_clash_task: allocate the two-actor "clash"/tie-up task
 * (handler actor_clash_state @0x02226eb8) for this actor, wire up its actor
 * pointers, prime the clash substate and move the actor into state 2.
 * ABI: single arg r4 = entity.
 *
 *   task = func_0220ff72(*(s32*)0x06099580, actor_clash_state);  (spawn task)
 *   task->unk8->unk10 = entity;                       (A = this actor)
 *   task->unk8->unk14 = g_060995B0[entity[0x4b]]->unk8; (B = partner actor)
 *   actor_init_from_cfg(entity, (void*)0x060900fc);          (clash init table)
 *   entity[0xd4] = 0xa;
 *   entity[0xd8] = 2;
 *
 * func_0220ff72 is the task spawner (id r4, handler r5 -> task*); its 2nd arg is
 * the actor_clash_state function pointer built by the PIC mova/add idiom
 * (0x022490ac + 0xfffdde0c = 0x02226eb8, resolved with ptr_resolve). actor_init_from_cfg
 * takes (entity, ptr) as in the sibling 0x2485b8. fidelity: HIGH - hand-traced
 * vs m2c_fn --asm. */
extern void *task_spawn_child_work40(s32 id, void *handler);      /* task spawner */
extern void  actor_init_from_cfg(void *entity, void *table);
extern void  actor_clash_state(void *task);             /* 0x02226eb8 */

void enemy_spawn_clash_task(void *entity) {
    void *task = task_spawn_child_work40(*(s32 *)(uintptr_t)0x06099580,
                               (void *)actor_clash_state);
    MP(TASK_PTR(task, work), 0x10) = entity;
    MP(TASK_PTR(task, work), 0x14) = TASK_PTR(((void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR)[M8(entity, 0x4B)], work);
    actor_init_from_cfg(entity, (void *)(uintptr_t)0x060900FC);
    ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_GRAPPLE;
    ACTOR_M32(entity, behavior_phase) = 2;
}

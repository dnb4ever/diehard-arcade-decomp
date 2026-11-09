#include "dh_work.h"
#include "dh.h"
/* 0x022934a (actor_hit_spawn_enter) - spawn a linked partner/effect actor, then enter the hit
 * state (task=r4, entity=r5). Allocates an object from the spawn pool g_06099580 using the ROM
 * template @0x02226EB8, links the spawned entity (obj->unk8) back to this actor (its unk10) and to
 * the slot object g_060995B0[entity->unk4B]->unk8 (its unk14), stamps request unk43 = 25, then
 * tail-calls actor_hit_state_enter(task, entity).
 * fidelity: HIGH - spawn via func_0220ff72(g_06099580, template@0x02226EB8) resolved via
 * ptr_resolve (0x229358 - 0x24a0); spawn-call bsrf -> 0x20ff72; the function restores its frame
 * and jumps to the adjacent handler at 0x2293b8 (actor_hit_state_enter) - rendered as a tail call.
 * unk4B/43 byte, unk10/14 long. */
extern void *task_spawn_child_work40(s32 ctx, void *template);  /* spawn/alloc object */
extern void actor_hit_state_enter(void *task, void *entity);  /* 0x2293b8 */
extern s32  g_06099580;      /* spawn pool context */
extern void *g_actor_slot_tasks[];   /* per-slot object pointers */

void actor_hit_spawn_enter(void *task, void *entity) {
    void *obj = task_spawn_child_work40(g_06099580, (void *)0x0602AEB8 /* PC-relative template in HWRAM */);
    void *spawned = MP(obj, 8);
    MP(spawned, 0x10) = entity;
    MP(spawned, 0x14) = TASK_PTR(g_actor_slot_tasks[ACTOR_M8(entity, paired_slot)], work);
    ACTOR_WORK(entity)->state_handler = 25;
    actor_hit_state_enter(task, entity);
}

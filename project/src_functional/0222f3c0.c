/* actor_publish_linked_slot_value [0x0222f3c0]: Writes the source link byte into the live actor slot's companion work word. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222f3c0 - propagate an actor's value to its paired object. slot = entity->unk34. If the paired
 * slot is live (g_06099590[slot] != 0), write entity->unk5D into the paired object's entity:
 * g_060995B0[slot]->unk8->unkB0 = entity->unk5D.
 * fidelity: HIGH - slot byte, liveness/pointer tables long, unkB0 long, unk5D byte (sign-extended).
 * g_06099590 / g_060995B0 are two arrays in the slot-table block based at 0x06099584 (+0xC / +0x2C). */
extern s32  g_actor_slot_ids[];   /* per-slot liveness */
extern void *g_actor_slot_tasks[];  /* per-slot object pointers */

void actor_publish_linked_slot_value(void *entity) {
    s32 slot = M8(entity, 0x34);

    if (g_actor_slot_ids[slot] != 0)
        M32(TASK_PTR(g_actor_slot_tasks[slot], work), 0xB0) = M8(entity, 0x5D);
}

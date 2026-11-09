/* actor_mark_slots_for_refresh [0x0222a708]: Marks live actor and effect slots for their next refresh. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222a708 (leafC) - per-frame flag refresh over two slot arrays. For player/entity slots
 * 1..6: if the active flag g_06099590[k] is set, force g_060995B0[k]->unk8->field3C = 0x10.
 * Then for 8 entries: if g_060E0F0C[k] is set, store 5 into g_060E0F2C[k].
 * fidelity: HIGH - the two compiler-unrolled loops flattened; all slots 32-bit. Leaf. */
extern s32   g_actor_slot_ids[];   /* active-slot flags (base 0x06099584 + 0xC)  */
extern void *g_actor_slot_tasks[];   /* slot object pointers (base 0x06099584 + 0x2C) */
extern s32   g_060E0F0C[];   /* flags (base 0x060E0E88 + 0x84) */
extern s32   g_060E0F2C[];   /* targets (base 0x060E0E88 + 0xA4) */
void actor_mark_slots_for_refresh(void) {
    s32 k;
    for (k = 1; k <= 6; k++)
        if (g_actor_slot_ids[k] != 0)
            M32(TASK_PTR(g_actor_slot_tasks[k], work), 0x3C) = 0x10;
    for (k = 0; k < 8; k++)
        if (g_060E0F0C[k] != 0)
            g_060E0F2C[k] = 5;
}

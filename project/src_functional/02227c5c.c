/* actor_clear_slot_without_count [0x02227c5c]: Clears occupied slot ID/task words while leaving the registry count unchanged. */
#include "dh.h"
/* 0x02227c5c  if slot [idx] at 0x06099584+0xC is set: clear slots +0xC and +0x2C (no count change).
 * fidelity: HIGH - exact (idx = arg0->unk34, 4-byte stride). */
extern u8 g_actor_slot_registry[];
void actor_clear_slot_without_count(void *arg0) {
    s32 idx = M8(arg0, 0x34);
    if (M32(&g_actor_slot_registry[0xC + idx * 4], 0) != 0) {
        M32(&g_actor_slot_registry[0xC + idx * 4], 0) = 0;
        M32(&g_actor_slot_registry[0x2C + idx * 4], 0) = 0;
    }
}

/* actor_release_slot [0x02227c22]: Clears the actor slot ID/task words and decrements the registry live count. */
#include "dh.h"
/* 0x02227c22  if slot [idx] at 0x06099584+0xC is set: decrement count @+0x8, clear slots +0xC and +0x2C.
 * fidelity: HIGH - exact (idx = arg0->unk34, 4-byte stride). */
extern u8 g_actor_slot_registry[];
void actor_release_slot(void *arg0) {
    s32 idx = M8(arg0, 0x34);
    if (M32(&g_actor_slot_registry[0xC + idx * 4], 0) != 0) {
        M32(g_actor_slot_registry, 0x8) = M32(g_actor_slot_registry, 0x8) - 1;
        M32(&g_actor_slot_registry[0xC + idx * 4], 0) = 0;
        M32(&g_actor_slot_registry[0x2C + idx * 4], 0) = 0;
    }
}

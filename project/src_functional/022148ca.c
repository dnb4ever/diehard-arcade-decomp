/* actor_clear_frame_pair_slots [0x022148ca]: Clears per-frame actor/effect pair scratch records through the cache mirror. */
#include "dh_band21.h"
/* 0x022148ca (clear_pair_slot_arrays) - zero-init two 8-element/0x20-stride arrays,
 * clearing words @+0 and @+0x10 of each element, plus a single global. Addresses are
 * taken through the 0x40000000 cache-through mirror:
 *   g_06099584 = 0; then 8x { g_060995D0[n].{+0,+10} = 0 }; 8x { g_060E0F6C[n].{+0,+10} = 0 }.
 * fidelity: HIGH - loop counts (r6:0x10 down by 2 -> 8 iters), 0x20 stride, two stores. */
void actor_clear_frame_pair_slots(void) {
    s32 i;

    MEM32(DH_ACTOR_SLOT_REGISTRY_ADDR_U | 0x40000000u) = 0;
    for (i = 0; i < 8; i++) {
        u32 b = (0x060995D0u | 0x40000000u) + (u32)i * 0x20;
        MEM32(b + 0x00) = 0;
        MEM32(b + 0x10) = 0;
    }
    for (i = 0; i < 8; i++) {
        u32 b = (0x060E0F6Cu | 0x40000000u) + (u32)i * 0x20;
        MEM32(b + 0x00) = 0;
        MEM32(b + 0x10) = 0;
    }
}

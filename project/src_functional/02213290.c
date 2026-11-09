/* memory_clear_vec4_blocks [0x02213290]: Writes four zero longs per block, walking backwards from end. */
#include "dh_band21.h"
/* 0x02213290 - zero-fill count*16 bytes (count 4-long blocks) ending at base+count*16,
 * writing backwards (mov.l 0,@-r4 x4 per iteration, dt count).
 * fidelity: HIGH - leaf; 16-byte stride, pre-decrement stores. */
void memory_clear_vec4_blocks(s32 base, s32 count) {
    volatile u32 *p = PTR(volatile u32, base + count * 16);
    s32 n;

    for (n = count; n > 0; n--) {
        *--p = 0;
        *--p = 0;
        *--p = 0;
        *--p = 0;
    }
}

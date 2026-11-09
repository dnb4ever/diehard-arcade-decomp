/* 0x022132ac  fill_vec4_array: fill `count` x 16-byte elements with `value`.
 * Walks descending from base+count*16, 4 longword pre-dec stores per element.
 * fidelity: faithful - r5<<4 end offset, four `mov.l r6,@-r4` per dt r3 iteration. Leaf. */
#include "dh_band21.h"

void fill_vec4_array(s32 *base, s32 count, s32 value) {
    s32 *p = base + count * 4;   /* base + count*16 bytes */
    do {
        *--p = value;
        *--p = value;
        *--p = value;
        *--p = value;
    } while (--count != 0);
}

/* 0x0221326c  copy_longs: copy `count` longwords src->dst.
 * fidelity: faithful - mov.l @r4+,r0 / mov.l r0,@r5 / dt r6 / bf/s / add #4,r5.
 * m2c drafted a spurious branch error (fn entry == loop top); hand-written. Leaf. */
#include "dh_band21.h"

void copy_longs(s32 *src, s32 *dst, s32 count) {
    do {
        *dst = *src++;
        dst++;
    } while (--count != 0);
}

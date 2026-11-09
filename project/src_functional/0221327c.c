/* 0x0221327c  zero_longs: zero-fill `count` longwords at dst.
 * fidelity: faithful - r0=0 / mov.l r0,@r4 / dt r5 / bf/s / add #4,r4. Leaf.
 * (m2c mis-scaled the s32* increment; corrected to one longword per step.) */
#include "dh_band21.h"

void zero_longs(s32 *dst, s32 count) {
    do {
        *dst++ = 0;
    } while (--count != 0);
}

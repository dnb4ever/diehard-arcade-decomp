/* 0x022134d8  matrix_copy_reverse_to: same as matrix_copy_reverse (0x2213474) but the
 * destination is the caller-supplied pointer `dst` (r4) instead of the fixed 0x060da040.
 * Copies 2 row-swapped matrices from the chain at *(void**)0x06098878 (src, then src-48),
 * then appends the 3-long translation vector from 0x06098b9c.
 * fidelity: faithful - r0=r4 dst, r3=2 dt loop, per-matrix row-swap, trailing 12-byte copy. Leaf. */
#include "dh_band21.h"

void matrix_copy_reverse_to(s32 *dst) {
    s32 *src = *PTR(s32 *, DH_MATRIX_STACK_CURSOR_ADDR);
    s32 *s = src;
    int m;

    for (m = 0; m < 2; m++) {
        dst[0]  = s[8];  dst[1]  = s[9];  dst[2]  = s[10]; dst[3]  = s[11];
        dst[4]  = s[4];  dst[5]  = s[5];  dst[6]  = s[6];  dst[7]  = s[7];
        dst[8]  = s[0];  dst[9]  = s[1];  dst[10] = s[2];  dst[11] = s[3];
        s = src - 12;
        dst += 12;
    }

    {
        s32 *t = PTR(s32, 0x06098b9c);
        dst[0] = t[0];
        dst[1] = t[1];
        dst[2] = t[2];
    }
}

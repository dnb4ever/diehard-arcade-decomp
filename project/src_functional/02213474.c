/* 0x02213474  matrix_copy_reverse: copy 2 matrices (48 bytes each) from the chain at
 * *(void**)0x06098878 into 0x060da040, row-swapping rows 0<->2 within each matrix and
 * taking the source matrices in reverse order (src, then src-48); then append a 3-long
 * translation vector from 0x06098b9c.
 * fidelity: faithful - r3=2 dt loop, per-matrix row-swap long copies, r1=src-48 reuse,
 *   dst+=48 per iter, trailing 12-byte copy. Leaf. */
#include "dh_band21.h"

void matrix_copy_reverse(void) {
    s32 *src = *PTR(s32 *, DH_MATRIX_STACK_CURSOR_ADDR);
    s32 *dst = PTR(s32, 0x060da040);
    s32 *s = src;
    int m;

    for (m = 0; m < 2; m++) {
        dst[0]  = s[8];  dst[1]  = s[9];  dst[2]  = s[10]; dst[3]  = s[11];
        dst[4]  = s[4];  dst[5]  = s[5];  dst[6]  = s[6];  dst[7]  = s[7];
        dst[8]  = s[0];  dst[9]  = s[1];  dst[10] = s[2];  dst[11] = s[3];
        s = src - 12;   /* src - 48 bytes */
        dst += 12;      /* dst + 48 bytes */
    }

    {
        s32 *t = PTR(s32, 0x06098b9c);
        dst[0] = t[0];
        dst[1] = t[1];
        dst[2] = t[2];
    }
}

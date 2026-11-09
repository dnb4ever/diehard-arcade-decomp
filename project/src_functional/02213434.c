/* 0x02213434  matrix_copy_rowswap: copy a 12-long (3x4) matrix with first and third
 * row blocks swapped. src = *(void**)0x06098878, dst = 0x060da040.
 *   dst[0..3]  = src[8..11]   (row +0x20 -> +0x00)
 *   dst[4..7]  = src[4..7]    (row +0x10 unchanged)
 *   dst[8..11] = src[0..3]    (row +0x00 -> +0x20)
 * fidelity: faithful - exact @(disp,Rn) long loads/stores in source order. Leaf. */
#include "dh_band21.h"

void matrix_copy_rowswap(void) {
    s32 *src = *PTR(s32 *, DH_MATRIX_STACK_CURSOR_ADDR);
    s32 *dst = PTR(s32, 0x060da040);

    dst[0]  = src[8];
    dst[1]  = src[9];
    dst[2]  = src[10];
    dst[3]  = src[11];

    dst[4]  = src[4];
    dst[5]  = src[5];
    dst[6]  = src[6];
    dst[7]  = src[7];

    dst[8]  = src[0];
    dst[9]  = src[1];
    dst[10] = src[2];
    dst[11] = src[3];
}

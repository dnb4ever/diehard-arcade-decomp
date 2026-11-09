/* 0x02213fb4  load_matrix_default: copy a 12-long (3x4, 48-byte) matrix from 0x0604ec64
 * into the current matrix buffer at *(void**)0x06098878.
 * fidelity: faithful - dst = *0x06098878, 12 longwords copied (asm unrolls 2 per step via
 *   @r1+ loads and @r2 stores with add #4,r2). Leaf. */
#include "dh_band21.h"

void load_matrix_default(void) {
    s32 *dst = *PTR(s32 *, DH_MATRIX_STACK_CURSOR_ADDR);
    s32 *src = PTR(s32, 0x0604ec64);
    int i;
    for (i = 0; i < 12; i++) {
        dst[i] = src[i];
    }
}

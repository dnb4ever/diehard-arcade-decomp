#include "decomp.h"
/* 0x0224a120  udiv32 - SH-2 runtime UNSIGNED 32-bit divide helper.
 * Compiler-internal register ABI: dividend in r1, divisor in r0; quotient returned
 * in r0. Body is `div0u; 32x(rotcl r1 / div1 r0,r2)` = unsigned division, quotient
 * built in r1. Divide-by-zero (r0==0) writes 0x44e to g_060e132c and returns 0.
 * fidelity: HIGH - exactly (u32)dividend/(u32)divisor. */
u32 udiv32(u32 dividend, u32 divisor) {
    if (divisor == 0) {
        MEMS32(0x060E132C) = 0x44E;
        return 0;
    }
    return dividend / divisor;
}

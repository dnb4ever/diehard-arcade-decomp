#include "decomp.h"
/* 0x0224a06c  sdiv32 - SH-2 runtime SIGNED 32-bit divide helper.
 * Compiler-internal register ABI: dividend in r1, divisor in r0; quotient returned
 * in r0. Body is the Hitachi idiom `mov #0,r2; div0s r2,r1; subc r3,r3; subc r2,r1;
 * div0s r0,r3; 32x(rotcl r1 / div1 r0,r3); addc r2,r1` = signed truncating division.
 * Divide-by-zero (r0==0) writes error code 0x44e to g_060e132c and returns 0.
 * fidelity: HIGH - the 32-step sequence is exactly signed dividend/divisor. */
s32 sdiv32(s32 dividend, s32 divisor) {
    if (divisor == 0) {
        MEMS32(0x060E132C) = 0x44E;
        return 0;
    }
    return dividend / divisor;
}

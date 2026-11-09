#include "decomp.h"
/* 0x0224a1c8  umod32 - SH-2 runtime UNSIGNED 32-bit modulo helper.
 * Compiler-internal register ABI: dividend in r1, divisor in r0; remainder returned
 * in r0. Body is `div0u; 32x(rotcl r1 / div1 r4,r3)` with the trailing
 * `bf; add r3,r0 / mov r3,r0` restoring-division remainder correction = unsigned
 * remainder. Divide-by-zero (r0==0) writes 0x44e to g_060e132c and returns 0.
 * fidelity: HIGH - exactly (u32)dividend%(u32)divisor. */
u32 umod32(u32 dividend, u32 divisor) {
    if (divisor == 0) {
        MEMS32(0x060E132C) = 0x44E;
        return 0;
    }
    return dividend % divisor;
}

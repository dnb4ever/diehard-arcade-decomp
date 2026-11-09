#include "decomp.h"
/* 0x0224aba0  shr29 - SH-2 unsigned right-shift-by-29 helper (value in r0, returned
 * in r0). Body: rotl x3 + and #7 = logical shift right by 29. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 29. */
u32 shr29(u32 x) {
    return x >> 29;
}

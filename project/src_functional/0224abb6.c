#include "decomp.h"
/* 0x0224abb6  shr31 - SH-2 unsigned right-shift-by-31 helper (value in r0, returned
 * in r0). Body: rotl x1 + and #1 = logical shift right by 31. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 31. */
u32 shr31(u32 x) {
    return x >> 31;
}

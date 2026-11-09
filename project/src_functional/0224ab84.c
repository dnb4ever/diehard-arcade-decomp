#include "decomp.h"
/* 0x0224ab84  shr27 - SH-2 unsigned right-shift-by-27 helper (value in r0, returned
 * in r0). Body: shlr x3 + shlr8 + shlr16 = logical shift right by 27. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 27. */
u32 shr27(u32 x) {
    return x >> 27;
}

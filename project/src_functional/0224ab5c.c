#include "decomp.h"
/* 0x0224ab5c  shr15 - SH-2 unsigned right-shift-by-15 helper (value in r0, returned
 * in r0). Body: shlr x7 + shlr8 = logical shift right by 15. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 15. */
u32 shr15(u32 x) {
    return x >> 15;
}

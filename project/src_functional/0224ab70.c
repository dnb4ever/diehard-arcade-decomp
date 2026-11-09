#include "decomp.h"
/* 0x0224ab70  shr23 - SH-2 unsigned right-shift-by-23 helper (value in r0, returned
 * in r0). Body: shlr x7 + shlr16 = logical shift right by 23. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 23. */
u32 shr23(u32 x) {
    return x >> 23;
}

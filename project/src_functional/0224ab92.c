#include "decomp.h"
/* 0x0224ab92  shr28 - SH-2 unsigned right-shift-by-28 helper (value in r0, returned
 * in r0). Body: rotl x4 + and #15 = logical shift right by 28. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 28. */
u32 shr28(u32 x) {
    return x >> 28;
}

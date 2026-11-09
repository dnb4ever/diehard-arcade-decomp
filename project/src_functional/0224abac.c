#include "decomp.h"
/* 0x0224abac  shr30 - SH-2 unsigned right-shift-by-30 helper (value in r0, returned
 * in r0). Body: rotl x2 + and #3 = logical shift right by 30. The trailing `mov.l @r15+,r2`
 * restores a register the shared dispatcher prologue spilled; it does not affect
 * the result. fidelity: HIGH - exactly (u32)x >> 30. */
u32 shr30(u32 x) {
    return x >> 30;
}

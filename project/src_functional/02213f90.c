/* 0x02213f90  sincos_lookup: table sin AND cos of one angle in a single pass.
 * idx = ((u16)(angle+8)) >> 4; sin = table[idx]<<1; cos = table[0x800 + idx]<<1 (s16, sign-ext).
 * The asm returns the pair in r4 (sin) and r5 (cos); modeled here with out-pointers.
 * fidelity: faithful - add#8/extu.w/shlr2 x2/shll, two mov.w @(r0,r3) loads (+0x800 for cos),
 *   two shll (<<1) results. Leaf. */
#include "dh_band21.h"

void sincos_lookup(s32 angle, s32 *out_sin, s32 *out_cos) {
    u16 idx = (u16)(angle + 8) >> 4;
    *out_sin = (s32)MEM16(TRIG_TABLE + idx * 2) << 1;
    *out_cos = (s32)MEM16(TRIG_TABLE + 0x800 + idx * 2) << 1;
}

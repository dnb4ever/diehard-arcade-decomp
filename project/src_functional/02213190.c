/* sin_lookup [0x02213190]: Reads the rounded unsigned-angle sine table and doubles the signed halfword result. */
/* 0x02213190  sin_lookup: table sine, 16.16-ish fixed.
 * idx = ((u16)(angle + 8)) >> 4; result = table[idx] << 1 (table is s16, sign-extended).
 * fidelity: faithful - matches add#8/extu.w/shlr2 x2/shll/mov.w @(r0,r3)/rts+shll (delay). Leaf. */
#include "dh_band21.h"

s32 sin_lookup(s32 angle) {
    u16 idx = (u16)(angle + 8) >> 4;
    return (s32)MEM16(TRIG_TABLE + idx * 2) << 1;
}

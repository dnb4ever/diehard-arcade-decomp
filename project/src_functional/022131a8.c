/* cos_lookup [0x022131a8]: Reads the rounded unsigned-angle cosine table and doubles the signed halfword result. */
/* 0x022131a8  cos_lookup: same table as sin_lookup, offset +0x800.
 * idx = ((u16)(angle + 8)) >> 4; result = table[0x800 + idx*2] << 1 (s16, sign-extended).
 * fidelity: faithful - matches add#8/extu.w/shlr2 x2/add 0x800/shll/mov.w @(r0,r3)/rts+shll. Leaf. */
#include "dh_band21.h"

s32 cos_lookup(s32 angle) {
    u16 idx = (u16)(angle + 8) >> 4;
    return (s32)MEM16(TRIG_TABLE + 0x800 + idx * 2) << 1;
}

/* 0x022130b4  atan2_fixed: fixed-point atan2 of (a, b) returning a 16-bit angle
 * (0x4000 = 90deg) using the SH-2 on-chip 64/32 divider and the s16 arctan
 * table @0x06078f28.
 *   flags: bit0 = a<0, bit1 = b<0, bit2 = |a|>|b| (after taking absolutes).
 *   ratio = (min<<16) / max   (64/32 DIVU);  t = atan_table[ratio >> 6];
 *   result = (octant[flags].sign ? -t : t) + octant[flags].base.
 * Returns 0 when both inputs are 0.
 * fidelity: HIGH - DVSR/DVDNTH/DVDNTL 64/32 divide, DVDNTUL readback, >>6 index,
 *   octant {sign,base} table (reproduced from ROM @0x02213124). Leaf. */
#include "dh_band21.h"

static const s32 atan2_octant[8][2] = {
    {1, 0x4000}, {0, 0x4000}, {0, 0xc000}, {1, 0xc000},
    {0, 0x0000}, {1, 0x8000}, {1, 0x0000}, {0, 0x8000},
};

s32 atan2_fixed(s32 a, s32 b) {
    s32 flags = 0;
    s32 angle;
    u32 ratio;

    if ((a | b) == 0) {
        return 0;
    }
    if (a < 0) { flags |= 1; a = -a; }
    if (b < 0) { flags |= 2; b = -b; }

    if ((u32)a > (u32)b) {
        MEM32(DIVU_DVSR)   = a;
        MEM32(DIVU_DVDNTH) = b >> 16;
        MEM32(DIVU_DVDNTL) = b << 16;
        flags |= 4;
    } else {
        MEM32(DIVU_DVSR)   = b;
        MEM32(DIVU_DVDNTH) = a >> 16;
        MEM32(DIVU_DVDNTL) = a << 16;
    }
    ratio = MEMU32(DIVU_DVDNTUL);

    angle = MEM16(0x06078f28 + (ratio >> 6) * 2);
    if (atan2_octant[flags][0] > 0) {
        angle = -angle;
    }
    return angle + atan2_octant[flags][1];
}

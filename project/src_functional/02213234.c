/* 0x02213234  sh2_hw_divide: SH-2 on-chip DIVU 64/32.
 * 64-bit dividend = (s64)num << 16  (high = num>>16 sign-ext via swap.w/exts.w, low = num<<16);
 * divisor = den; returns quotient (DVDNTUL). Effectively (num * 0x10000) / den, 16.16 fixed divide.
 * fidelity: faithful - matches DVSR/DVDNTH/DVDNTL writes + DVDNTUL read. Leaf. */
#include "dh_band21.h"

s32 sh2_hw_divide(s32 num, s32 den) {
    MEM32(DIVU_DVSR)   = den;
    MEM32(DIVU_DVDNTH) = num >> 16;   /* arithmetic: sign-extended high word */
    MEM32(DIVU_DVDNTL) = num << 16;   /* write triggers the 64/32 division   */
    return MEM32(DIVU_DVDNTUL);
}

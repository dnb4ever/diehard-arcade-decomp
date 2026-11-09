/* 0x02213248  sh2_hw_divide_32: SH-2 on-chip DIVU 32/32 variant.
 * divisor = (u32)den >> 4 (logical); dividend = num << 4 (write to DVDNT starts 32/32);
 * returns quotient (DVDNTUL).
 * fidelity: faithful - matches shlr2 x2 (divisor), shll2 x2 (dividend), DVSR/DVDNT writes, DVDNTUL read. Leaf. */
#include "dh_band21.h"

s32 sh2_hw_divide_32(s32 num, s32 den) {
    MEM32(DIVU_DVSR)  = (u32)den >> 4;  /* shlr2 x2: logical right shift */
    MEM32(DIVU_DVDNT) = num << 4;       /* shll2 x2; write triggers 32/32 division */
    return MEM32(DIVU_DVDNTUL);
}

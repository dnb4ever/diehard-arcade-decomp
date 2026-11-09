/* vdp2_commit_shadow_bank [0x02216582]: Same copy to VDP2 registers, preserving destination display-enable bit zero. */
#include "dh.h"
/* 0x02216582 - func_02216582(dst, src): same record copy as func_0221654a, but
 * word 0 keeps the destination's bit 0 (VDP2 TVMD-style display bit preserved):
 *   dst+0x00 = (src+0x00 & ~1) | (dst+0x00 & 1);
 *   dst+0x02 = src+0x02;  dst+0x06 = src+0x06;  dst+0x0C = src+0x0C;
 *   dst+0x0E = src+0x0E;                                   (16-bit, in this order)
 *   tail (braf) sh2_dmac_copy(src + 0x10, dst + 0x10, 68)
 * Called (bsr) by func_022165c6 as (0x25F80000 VDP2 regs, current-parity record).
 * fidelity: HIGH - hand-traced vs asm 0x216582..0x2165c4 (braf -> 0x2145d0). */
extern s32 sh2_dma0_copy_words(s32 src, s32 dst, s32 count);   /* 0x022145d0 sh2_dmac_copy */

s32 vdp2_commit_shadow_bank(u32 dst, u32 src)
{
    M16(dst, 0x00) = (s16)((M16(src, 0x00) & -2) | (M16(dst, 0x00) & 1));
    M16(dst, 0x02) = M16(src, 0x02);
    M16(dst, 0x06) = M16(src, 0x06);
    M16(dst, 0x0C) = M16(src, 0x0C);
    M16(dst, 0x0E) = M16(src, 0x0E);
    return sh2_dma0_copy_words((s32)(src + 0x10), (s32)(dst + 0x10), 68);
}

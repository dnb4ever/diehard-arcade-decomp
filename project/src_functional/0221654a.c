/* vdp2_copy_shadow_bank [0x0221654a]: Ordered halfword prefix copy then DMA 0x110-byte register-shadow tail. */
#include "dh.h"
/* 0x0221654a - func_0221654a(dst, src): copy the six VDP2 scroll/control words
 * of a 0x120-byte register-shadow record, then DMA the remaining block.
 *   dst+0x00 = src+0x00;  dst+0x02 = src+0x02;  dst+0x06 = src+0x06;
 *   dst+0x0C = src+0x0C;  dst+0x0E = src+0x0E;            (16-bit, in this order)
 *   tail (braf) sh2_dmac_copy(src + 0x10, dst + 0x10, 68)  (68 longs = 0x110 B)
 * Called (bra tail) by func_022165c6 as (other-parity record, *0x06098F08 shadow).
 * Sibling of func_02216582, which writes the same layout but keeps dst bit 0 of
 * word 0. Entry follows func_02216530's rts at 0x216548.
 * fidelity: HIGH - hand-traced vs asm 0x21654a..0x216580 (braf -> 0x2145d0). */
extern s32 sh2_dma0_copy_words(s32 src, s32 dst, s32 count);   /* 0x022145d0 sh2_dmac_copy */

s32 vdp2_copy_shadow_bank(u32 dst, u32 src)
{
    M16(dst, 0x00) = M16(src, 0x00);
    M16(dst, 0x02) = M16(src, 0x02);
    M16(dst, 0x06) = M16(src, 0x06);
    M16(dst, 0x0C) = M16(src, 0x0C);
    M16(dst, 0x0E) = M16(src, 0x0E);
    return sh2_dma0_copy_words((s32)(src + 0x10), (s32)(dst + 0x10), 68);
}

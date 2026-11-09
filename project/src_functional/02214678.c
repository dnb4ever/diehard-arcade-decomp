/* sh2_dmac_copy_aligned_blocks [0x02214678]: Leading longword ladder aligns source to sixteen, then DMAC0 control 0x5e11. */
#include "dh.h"
#include <stdint.h>
/* 0x02214678 - SH-2 DMAC channel-0 copy with a leading longword pre-roll that
 * aligns the source to a 16-byte boundary (CHCR tag 0x5E11). rem = src & 15; a
 * Duff-style ladder copies (4 - rem/4) longwords src->dst, decrementing the
 * longword count (tcr = count*4 bytes, less one per pre-roll longword), then both
 * pointers advance to (ptr - rem + 16). If, after alignment, src or dst is not
 * 4-aligned it records 0xA/0xC into g_060997A4/A8 and calls the fault thunk
 * *g_060902A0, then DMAs the remaining count. Returns 1 when both are 4-aligned,
 * else 0. fidelity: HIGH - verified against asm; the fault call site pushes exactly
 * three stack args (table 0x06053D98, src, dst) and cleans 12 bytes (add #12,r15),
 * so the (s32,s32,s32) signature is unambiguous. The only non-static piece is the
 * runtime-installed thunk body at *g_060902A0, which is outside this TU. */
extern volatile u32 reg_ffffff80, reg_ffffff84, reg_ffffff88, reg_ffffff8c, reg_ffffffb0;
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, s32, s32);

s32 sh2_dmac_copy_aligned_blocks(s32 src, s32 dst, s32 count) {
    s32 rem = src & 15;
    s32 tcr = count << 2;
    s32 i = 0;

    if (rem != 0) {
        s32 *s = (s32 *)(uintptr_t)src;
        s32 *d = (s32 *)(uintptr_t)dst;
        switch ((u32)rem >> 2) {
        case 0: *d++ = *s++; tcr--; /* fallthrough */
        case 1: *d++ = *s++; tcr--; /* fallthrough */
        case 2: *d++ = *s++; tcr--; /* fallthrough */
        case 3: *d = *s;     tcr--; break;
        }
        src = (src - rem) + 16;
        dst = (dst - rem) + 16;
    }
    if ((src & 3) || (dst & 3)) {
        g_ui_cursor_column = 0xA;
        g_ui_cursor_row = 0xC;
        g_060902A0(0x06053D98, src, dst);
    }
    while (!(reg_ffffffb0 & 4) && !(reg_ffffff8c & 2)) {
        if (++i >= 0x10000)
            break;
    }
    reg_ffffff8c = 0;
    (void)reg_ffffffb0; /* ROM DMAOR read between CHCR clear and enable */
    reg_ffffffb0 = 1;
    reg_ffffff80 = src & ~3;
    reg_ffffff84 = dst & ~3;
    reg_ffffff88 = tcr;
    (void)reg_ffffff8c; /* ROM read before final CHCR clear/kick */
    reg_ffffff8c = 0;
    reg_ffffff8c = 0x5E11;
    return ((src & 3) || (dst & 3)) ? 0 : 1;
}

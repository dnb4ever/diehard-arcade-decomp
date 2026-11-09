/* sh2_dmac_copy_halfwords [0x02214510]: DMAC0 control 0x5611; odd-address diagnostic then masked-even transfer. */
#include "dh.h"
/* 0x02214510 - SH-2 DMAC channel-0 copy used as a 16-bit (halfword) block copier
 * (CHCR tag 0x5611). If src or dst is odd it records 0xA/0xA into g_060997A4/A8 and
 * calls the alignment-fault thunk *g_060902A0(0x06053D70, src, dst); the copy
 * is still programmed with the addresses masked even. Waits (cap 0x10000) on
 * DMAOR AE / CHCR0 TE, programs SAR0=src&~1, DAR0=dst&~1, TCR0=count, kicks
 * CHCR0=0x5611. Returns 1 when both addresses were even, else 0. Used by
 * heap_realloc (0x02214bec) to move block payloads. fidelity: HIGH - the fault
 * thunk receives three stack words: format 0x06053D70, src, dst; r4 is unused. */
extern volatile u32 reg_ffffff80, reg_ffffff84, reg_ffffff88, reg_ffffff8c, reg_ffffffb0;
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, s32, s32);

s32 sh2_dmac_copy_halfwords(s32 src, s32 dst, s32 count) {
    s32 bad = ((src & 1) != 0) || ((dst & 1) != 0);
    s32 i = 0;

    if (bad) {
        g_ui_cursor_column = 0xA;
        g_ui_cursor_row = 0xA;
        g_060902A0(0x06053D70, src, dst);
    }
    while (!(reg_ffffffb0 & 4) && !(reg_ffffff8c & 2)) {
        if (++i >= 0x10000)
            break;
    }
    reg_ffffff8c = 0;
    (void)reg_ffffffb0; /* ROM DMAOR read between CHCR clear and enable */
    reg_ffffffb0 = 1;
    reg_ffffff80 = src & ~1;
    reg_ffffff84 = dst & ~1;
    reg_ffffff88 = count;
    (void)reg_ffffff8c; /* ROM read before final CHCR clear/kick */
    reg_ffffff8c = 0;
    reg_ffffff8c = 0x5611;
    return bad ? 0 : 1;
}

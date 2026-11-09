/* sh2_dma0_copy_words [0x022145d0]: Aligns source/destination and programs channel zero for word transfers, preserving volatile read order. */
#include "dh.h"
/* 0x022145d0 (sh2_dmac_copy) - start an SH-2 on-chip DMAC channel-0 transfer.
 * src=arg0, dst=arg1, count=arg2 (TCR). Both addresses must be 4-byte aligned; if
 * either has low bits set it records 0xA/0xB into g_060997A4/A8 and calls the
 * alignment-fault handler thunk at *g_060902A0(0x06053D84, src, dst) (the DMA is
 * still programmed). Waits (cap 0x10000) until DMAOR bit2 (AE) or CHCR0 bit1 (TE)
 * is observed, then programs SAR0/DAR0/TCR0 and kicks CHCR0 = 0x5A11. Returns 1 when
 * both addresses were aligned, 0 otherwise.
 * SH-2 on-chip regs: SAR0 0xFFFFFF80, DAR0 0xFFFFFF84, TCR0 0xFFFFFF88,
 * CHCR0 0xFFFFFF8C, DMAOR 0xFFFFFFB0.
 * The formatter takes three stack words; stale r4 is ignored.
 * fidelity: HIGH - faithful to the asm (word accesses; src/dst masked ~3). */
extern volatile u32 reg_ffffff80;   /* DMAC0 SAR0  - source address      */
extern volatile u32 reg_ffffff84;   /* DMAC0 DAR0  - destination address */
extern volatile u32 reg_ffffff88;   /* DMAC0 TCR0  - transfer count      */
extern volatile u32 reg_ffffff8c;   /* DMAC0 CHCR0 - channel control     */
extern volatile u32 reg_ffffffb0;   /* DMAC  DMAOR - operation register  */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, s32, s32);  /* alignment-fault handler */

s32 sh2_dma0_copy_words(s32 src, s32 dst, s32 count) {
    s32 bad = ((src & 3) != 0) || ((dst & 3) != 0);
    s32 i = 0;

    if (bad) {
        g_ui_cursor_column = 0xA;
        g_ui_cursor_row = 0xB;
        g_060902A0(0x06053D84, src, dst);
    }
    while (!(reg_ffffffb0 & 4) && !(reg_ffffff8c & 2)) {
        if (++i >= 0x10000)
            break;
    }
    reg_ffffff8c = 0;           /* CHCR0 clear                 */
    (void)reg_ffffffb0;     /* ROM 06018638: volatile DMAOR observation */
    reg_ffffffb0 = 1;           /* DMAOR enable                */
    reg_ffffff80 = src & ~3;    /* SAR0                        */
    reg_ffffff84 = dst & ~3;    /* DAR0                        */
    reg_ffffff88 = count;       /* TCR0                        */
    (void)reg_ffffff8c;     /* ROM 06018650: volatile CHCR0 observation */
    reg_ffffff8c = 0;           /* CHCR0 clear                 */
    reg_ffffff8c = 0x5A11;      /* CHCR0 enable+auto-request   */
    return bad ? 0 : 1;
}

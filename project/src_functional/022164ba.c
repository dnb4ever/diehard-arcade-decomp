/* 0x022164ba  flush_dirty_dma_records: walk the 24-byte record array @0x06098f0c
 * (count = *(s32*)0x0609950c). For each record whose s32[+4] flag == 1, issue a DMAC copy
 *   sh2_dmac_copy(rec[+8], rec[+16], rec[+20])  and clear the flag. Then reset_displist_state().
 * fidelity: faithful - bra-to-condition loop (cmp/ge count), cmp/eq #1 flag gate, jsr @r11
 *   (r11 = mova+0xffffe100 = 0x022145d0 = sh2_dmac_copy), flag clear, tail bra to 0x022162fc. */
#include "dh_band21.h"

extern void sh2_dmac_copy(s32 r4, s32 r5, s32 r6);   /* 0x022145d0 SH-2 on-chip DMAC ch0 */
extern void reset_displist_state(void);              /* 0x022162fc */

void flush_dirty_dma_records(void) {
    s8 *rec = PTR(s8, 0x06098f0c);
    s32 i = 0;

    while (i < MEM32(0x0609950c)) {
        if (*(s32 *)(rec + 4) == 1) {
            sh2_dmac_copy(*(s32 *)(rec + 8), *(s32 *)(rec + 16), *(s32 *)(rec + 20));
            *(s32 *)(rec + 4) = 0;
        }
        i++;
        rec += 24;
    }

    reset_displist_state();
}

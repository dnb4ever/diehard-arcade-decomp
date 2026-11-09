/* sh2_dmac_copy_unchecked [0x022144b8]: DMAC0 channel control 0x5211 with bounded busy wait and raw addresses. */
#include "dh.h"
/* 0x022144b8 (sh2_dmac_kick) - start an SH-2 on-chip DMAC channel-0 transfer with no
 * alignment check (plain sibling of sh2_dmac_copy 0x022145d0). Spin until DMAOR bit2
 * (AE) or CHCR0 bit1 (TE) is seen (cap 0x10000), then program SAR0/DAR0/TCR0 and kick
 * CHCR0 = 0x5211. Always returns 1.
 * SH-2 on-chip regs: SAR0 0xFFFFFF80, DAR0 0xFFFFFF84, TCR0 0xFFFFFF88,
 * CHCR0 0xFFFFFF8C, DMAOR 0xFFFFFFB0.
 * fidelity: HIGH - word reg accesses; volatile pre-write reads retained in ROM order. */
extern volatile u32 reg_ffffff80;   /* DMAC0 SAR0  */
extern volatile u32 reg_ffffff84;   /* DMAC0 DAR0  */
extern volatile u32 reg_ffffff88;   /* DMAC0 TCR0  */
extern volatile u32 reg_ffffff8c;   /* DMAC0 CHCR0 */
extern volatile u32 reg_ffffffb0;   /* DMAC  DMAOR */

s32 sh2_dmac_copy_unchecked(s32 src, s32 dst, s32 count) {
    s32 i = 0;

    while (!(reg_ffffffb0 & 4) && !(reg_ffffff8c & 2)) {
        if (++i >= 0x10000)
            break;
    }
    reg_ffffff8c = 0;           /* CHCR0 clear  */
    (void)reg_ffffffb0; /* ROM DMAOR read between CHCR clear and enable */
    reg_ffffffb0 = 1;           /* DMAOR enable */
    reg_ffffff80 = src;         /* SAR0         */
    reg_ffffff84 = dst;         /* DAR0         */
    reg_ffffff88 = count;       /* TCR0         */
    (void)reg_ffffff8c; /* ROM read before final CHCR clear/kick */
    reg_ffffff8c = 0;           /* CHCR0 clear  */
    reg_ffffff8c = 0x5211;      /* CHCR0 kick   */
    return 1;
}

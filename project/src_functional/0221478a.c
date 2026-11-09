/* scu_dma0_transfer [0x0221478a]: SCU level-0 source/destination/count registers and mode-7 start. */
#include "dh_band21.h"
/* 0x0221478a (scu_dma0_transfer) - program and start an SCU DMA level-0 transfer.
 * Spin on the DMA status reg (0x25FE007C & 0x10010, cap 0x10000) until the channel
 * is free, then set D0R=src, D0W=dst, D0C=count, D0AD=0x101, D0MD=7, D0EN=0x101.
 * SCU L0 regs: D0R 0x25FE0000, D0W 0x25FE0004, D0C 0x25FE0008, D0AD 0x25FE000C,
 * D0EN 0x25FE0010, D0MD 0x25FE0014, status 0x25FE007C.
 * fidelity: HIGH - word accesses; matches the asm. */
void scu_dma0_transfer(s32 src, s32 dst, s32 count) {
    s32 i = 0;

    while (MEM32(SCU_DSTA_ADDR_U) & 0x10010) {
        if (++i >= 0x10000)
            break;
    }
    MEM32(SCU_D0EN_ADDR_U) = 0;       /* D0EN stop            */
    MEM32(SCU_D0R_ADDR_U) = src;     /* D0R  source          */
    MEM32(SCU_D0W_ADDR_U) = dst;     /* D0W  destination     */
    MEM32(SCU_D0C_ADDR_U) = count;   /* D0C  count           */
    MEM32(SCU_D0AD_ADDR_U) = 0x101;   /* D0AD add-value mode  */
    MEM32(SCU_D0MD_ADDR_U) = 7;       /* D0MD mode            */
    MEM32(SCU_D0EN_ADDR_U) = 0x101;   /* D0EN start           */
}

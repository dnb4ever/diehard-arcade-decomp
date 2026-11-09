/* scu_dma_stack_submit [0x0221482e]: Pops descriptor, sets high end flag and starts SCU indirect DMA mode 0x01000007. */
#include "dh_band21.h"
/* 0x0221482e (scu_dma_push) - pop a 12-byte descriptor off the SCU-DMA descriptor
 * stack (ptr g_06098558, base g_0609855C) and, if the stack has not underflowed the
 * base, flag the descriptor's 3rd word with 0x80000000 and kick an SCU DMA L0
 * write: wait for the channel (0x25FE007C & 0x10010, cap 0x10000), then D0EN=0,
 * D0W=g_0609855C, D0AD=0x101, D0MD=0x01000007, D0EN=0x101. Only D0W is reprogrammed.
 * fidelity: MED - control flow and register writes match the asm; descriptor-stack
 * semantics (what g_06098558/855C point at) inferred from usage. */
extern u32 g_06098558;   /* SCU DMA descriptor stack pointer (SH-2 address) */
extern u32 g_0609855C;   /* descriptor stack base / D0W write value         */

void scu_dma_stack_submit(void) {
    u32 sp = g_06098558 - 0xC;
    s32 i = 0;

    g_06098558 = sp;
    if (sp >= g_0609855C) {
        MEMU32(sp + 8) |= 0x80000000u;
        while (MEM32(SCU_DSTA_ADDR_U) & 0x10010) {
            if (++i >= 0x10000)
                break;
        }
        MEM32(SCU_D0EN_ADDR_U) = 0;             /* D0EN stop  */
        MEM32(SCU_D0W_ADDR_U) = g_0609855C;    /* D0W        */
        MEM32(SCU_D0AD_ADDR_U) = 0x101;         /* D0AD       */
        MEM32(SCU_D0MD_ADDR_U) = 0x01000007;    /* D0MD       */
        MEM32(SCU_D0EN_ADDR_U) = 0x101;         /* D0EN start */
    }
}

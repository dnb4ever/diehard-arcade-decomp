/* scu_dma_stack_push [0x02214818]: Appends 12-byte descriptor and advances 06098558 by twelve. */
#include "dh_band21.h"
/* 0x02214818 (scu_dma_stack_push) - push a 12-byte descriptor {a2,a1,a0} onto the
 * SCU-DMA descriptor stack at g_06098558 (writes words @0/@4/@8) and advance the
 * stack pointer by 0xC. Pairs with scu_dma_push (0x0221482e, which pops) and
 * scu_dma_stack_reset (0x022147cc).
 * fidelity: HIGH - three word stores + pointer advance. Leaf. */
extern u32 g_06098558;   /* descriptor stack current pointer */

void scu_dma_stack_push(s32 a0, s32 a1, s32 a2) {
    u32 p = g_06098558;

    MEM32(p + 0) = a2;
    MEM32(p + 4) = a1;
    MEM32(p + 8) = a0;
    g_06098558 = p + 0xC;
}

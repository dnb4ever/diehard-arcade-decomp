/* scu_dma_stack_reset [0x022147cc]: Both descriptor cursors set to 060f8000. */
#include "dh.h"
/* 0x022147cc (scu_dma_stack_reset) - reset the SCU-DMA descriptor stack: set both
 * the base (g_0609855C) and the current pointer (g_06098558) to the top-of-stack
 * address 0x060F8000. Pairs with scu_dma_stack_push (0x02214818) and
 * scu_dma_push (0x0221482e).
 * fidelity: HIGH - two word stores. Leaf. */
extern u32 g_06098558;   /* descriptor stack current pointer */
extern u32 g_0609855C;   /* descriptor stack base            */

void scu_dma_stack_reset(void) {
    g_0609855C = 0x060F8000;
    g_06098558 = 0x060F8000;
}

/* vdp1_upload_command_lists [0x02214f90]: Walks 64 priority lists and builds SCU DMA descriptors targeting VDP1 VRAM. */
#include "dh_band21.h"
/* 0x02214f90 (vdp1_build_cmdlist) - stage the frame`s VDP1 command data into VDP1
 * VRAM (0x25C00000) over the SCU DMA descriptor stack. If drawing is active
 * (*0x06099578 != 0) it first waits for VDP1 EDSR bit1 (0x25D00010 & 2). Resets the
 * descriptor stack (func_022147cc), then for each of 64 per-priority lists (heads at
 * 0x060BAFB4, stepping back 12 bytes) it walks the singly-linked node list (next @0)
 * and for every node pushes a 32-byte DMA descriptor (src = node->field4, dst =
 * running VDP1 cursor) via func_02214818, clearing the list head afterwards. Finally
 * it pushes a 2-unit descriptor from 0x060C3CC0 and tail-calls func_0221482e to kick
 * the SCU DMA. fidelity: HIGH - the 64-list walk, 32-byte stride and arg order
 * verified against the asm (jsr delay-slot sets src for the pending push). */
extern void scu_dma_stack_reset(void);                        /* scu_dma_stack_reset */
extern void scu_dma_stack_push(s32 src, s32 dst, s32 size);  /* scu_dma_stack_push  */
extern void scu_dma_stack_submit(void);                        /* scu_dma_push (kick) */

void vdp1_upload_command_lists(void) {
    s32 *list = (s32 *)(uintptr_t)0x060BAFB4;
    s32 dst = 0x25C00000;
    s32 k;

    if (MEM32(0x06099578) != 0) {
        while ((MEMU16(VDP1_EDSR_ADDR) & 2) == 0) {}
    }
    scu_dma_stack_reset();

    for (k = 64; k != 0; k--) {
        void *node = *(void **)list;
        if (node != 0) {
            do {
                scu_dma_stack_push(*(s32 *)((char *)node + 4), dst, 32);
                dst += 32;
                node = *(void **)node;
            } while (node != 0);
            *(void **)list = (void *)0;
        }
        list -= 3;
    }
    scu_dma_stack_push(0x060C3CC0, dst, 2);
    scu_dma_stack_submit();
}

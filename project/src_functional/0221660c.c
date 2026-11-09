/* display_flush_transfer_banks [0x0221660c]: Four 0x18-byte descriptors each DMA current buffer out and next source in. */
/* 0x0221660c  flush_draw_lists_dma: for each of the four draw-list descriptors
 * at 0x06099514 (stride 0x18), if its +4 enable word is set, DMA the current
 * frame buffer out and DMA the next frame source in (double-buffered by the
 * index @0x06099574). Uses dma_ch0_transfer (0x022145d0).
 *   cur = *0x06099574;  nxt4 = (1 ^ cur) * 4;
 *   per descriptor d:  if (d[4]) {
 *       dma(d[8 + cur*4],  d[0x10],      d[0x14]);
 *       dma(*src_ptr[i],   d[8 + nxt4],  d[0x14]); }
 * src_ptr[] = {0x060b8b90, 0x060b8b94, 0x060bac98, 0x060bac9c}.
 * fidelity: faithful - 4 unrolled descriptors folded to a loop, r12=(1^cur)*4,
 *   two dma_ch0_transfer calls guarded by the enable word. */
#include "dh_band21.h"

extern s32 dma_ch0_transfer(s32 src, s32 dst, s32 count);

void display_flush_transfer_banks(void) {
    static const u32 src_ptr[4] = {0x060b8b90, 0x060b8b94, 0x060bac98, 0x060bac9c};
    s32 cur  = MEM32(0x06099574);
    s32 nxt4 = (1 ^ cur) * 4;
    u32 d = 0x06099514;
    int i;

    for (i = 0; i < 4; i++, d += 0x18) {
        if (MEM32(d + 4) != 0) {
            dma_ch0_transfer(MEM32(d + 8 + cur * 4), MEM32(d + 0x10), MEM32(d + 0x14));
            dma_ch0_transfer(MEM32(src_ptr[i]), MEM32(d + 8 + nxt4), MEM32(d + 0x14));
        }
    }
}

/* text_upload_work_buffers [0x02222efc]: Two 0x100-word DMA uploads to 25e7c000/25e7c800. */
#include "dh.h"
/* 0x02222efc  draw-callback thunk: DMA/copy 0x100 words of the text working
 * buffer to VRAM: 0x060997B0 -> 0x25E7C000 and 0x06099FB0 -> 0x25E7C800 via
 * func_022145d0; returns the second call's result. */
extern s32 sh2_dma0_copy_words(s32 src, s32 dst, s32 count);
s32 text_upload_work_buffers(void) {
    sh2_dma0_copy_words(0x060997B0, 0x25E7C000, 0x100);
    return sh2_dma0_copy_words(0x06099FB0, 0x25E7C800, 0x100);
}

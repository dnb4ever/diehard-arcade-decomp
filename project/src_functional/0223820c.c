/* hud_upload_shared_assets [0x0223820c]: Uploads fixed HUD palettes/patterns and clears the shared background work area. */
#include "dh.h"
/* 0x0223820c (graphics setup, no args) - bulk-copy a set of ROM tables to VDP
 * registers / VRAM via the generic transfer helper func_022145d0(src, dst, len)
 * (and one func_02213290(dst, len) fill).  Pure sequence of fixed transfers;
 * the source addresses are cache-through ROM mirrors (0x224xxxxx), the
 * destinations are VDP2 register/VRAM space (0x25Exxxxx / 0x25Fxxxxx).
 * fidelity: HIGH. func-ptr r14 = mova(.Lpool_238214)+0xFFFDC3BC = 0x022145d0;
 * bsrf 0x238244 -> func_02213290. */
extern void sh2_dma0_copy_words(u32 src, u32 dst, u32 len);
extern void memory_clear_vec4_blocks(u32 dst, u32 len);

void hud_upload_shared_assets(void) {
    sh2_dma0_copy_words(0x224170A0, 0x25F00300, 8);
    sh2_dma0_copy_words(0x224170C0, 0x25F00320, 8);
    sh2_dma0_copy_words(0x224170E0, 0x25F00340, 8);
    sh2_dma0_copy_words(0x224170A0, 0x25F00360, 8);
    sh2_dma0_copy_words(0x224148A0, 0x25E60600, 0x80);
    memory_clear_vec4_blocks(0x25E66B00, 64);
    sh2_dma0_copy_words(0x22414840, 0x25F00280, 8);
    sh2_dma0_copy_words(0x22414860, 0x25F002A0, 8);
    sh2_dma0_copy_words(0x22414880, 0x25F002C0, 8);
    sh2_dma0_copy_words(0x2240A540, 0x25E64000, 0x500);
    sh2_dma0_copy_words(0x22413D40, 0x25E65400, 0x2C0);
    sh2_dma0_copy_words(0x2241AE00, 0x25F002E0, 8);
    sh2_dma0_copy_words(0x22417100, 0x25E66F00, 0xF40);
}

/* Round 4: ROM PIC anchors 06026dd8..26e04 / 06026e90..26ebc install HWRAM callback addresses in writable image slots; cart addresses were incorrect. Evidence: work/decomp4/image_slots.asm. */
/* hud_setup_large_font [0x06026e44]: Uploads large-font assets and palettes and installs its four text callbacks. */
#include "dh.h"
/* 0x06026e44  VDP2 setup for the alternate (larger) text/overlay mode. Sibling of func_02222d3c:
 * DMA the mode's palette (selected by `mode & 7`) to VDP2 colour RAM, upload a 0x1000-byte tile
 * block, clear the working buffer, install this mode's four draw callbacks and flag ready.
 * fidelity: HIGH - DMA source/dest/len and all four callback targets resolved from the ROM. */
extern u32  g_060909A0[];   /* per-mode palette source pointers */
extern void *g_06090294;    /* draw callback slot 0 */
extern void *g_06090298;    /* draw callback slot 1 */
extern void *g_0609029C;    /* draw callback slot 2 */
extern void *g_060902A0;    /* draw callback slot 3 */
extern s32  g_060997AC;     /* mode-ready flag / current palette index */
extern void sh2_dma0_copy_words(u32 src, u32 dst, u32 len);
extern void vdp2_apply_text_layer_commands(u32 arg0);
extern void vdp2_pack_layer_addresses(s32 a, u32 b, u32 c, u32 d, u32 e);
void hud_setup_large_font(s32 mode) {
    sh2_dma0_copy_words(g_060909A0[mode & 7], 0x25F00220, 8);
    sh2_dma0_copy_words(0x22400000, 0x25F003E0, 8);
    vdp2_apply_text_layer_commands(0x06075674);
    sh2_dma0_copy_words(0x22402020, 0x25E60000, 0x1000);
    vdp2_pack_layer_addresses(3, 0x25E7C000, 0x25E7C000, 0x25E7C000, 0x25E7C000);
    g_06090294 = (void *)(uintptr_t)0x06026ed6;
    g_06090298 = (void *)(uintptr_t)0x06026efc;
    g_0609029C = (void *)(uintptr_t)0x06027080;
    g_060902A0 = (void *)(uintptr_t)0x06027460;
    g_060997AC = 1;
}

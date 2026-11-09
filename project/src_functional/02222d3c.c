/* Round 4: ROM PIC anchors 06026dd8..26e04 / 06026e90..26ebc install HWRAM callback addresses in writable image slots; cart addresses were incorrect. Evidence: work/decomp4/image_slots.asm. */
/* hud_setup_small_font [0x06026d3c]: Uploads small-font assets and palettes and installs its four text callbacks. */
#include "dh.h"
/* 0x06026d3c  VDP2 setup for a text/overlay mode: DMA the mode's palette (selected by `mode & 7`)
 * and gradient tables into VDP2 colour RAM, upload the tile pattern/attribute blocks, clear the
 * working buffer, then install the four draw callbacks for this mode and flag it ready
 * (0x060997AC = 1). Variant func_02222e44 drives the larger-font mode.
 * fidelity: HIGH - DMA source/dest/len and all four callback targets resolved from the ROM.
 * The draw-callback slots hold absolute code pointers (two land mid-thunk-block at 0x02222e..). */
extern u32  g_06090980[];   /* per-mode palette source pointers */
extern void *g_06090294;    /* draw callback slot 0 */
extern void *g_06090298;    /* draw callback slot 1 */
extern void *g_0609029C;    /* draw callback slot 2 */
extern void *g_060902A0;    /* draw callback slot 3 */
extern s32  g_060997AC;     /* mode-ready flag / current palette index */
extern void sh2_dma0_copy_words(u32 src, u32 dst, u32 len);
extern void vdp2_apply_text_layer_commands(u32 arg0);
extern void vdp2_pack_layer_addresses(s32 a, u32 b, u32 c, u32 d, u32 e);
void hud_setup_small_font(s32 mode) {
    u32 *src;
    u32 dst;
    sh2_dma0_copy_words(g_06090980[mode & 7], 0x25F00220, 8);
    src = &g_06090980[1];
    dst = 0x25F00240;
    do {
        sh2_dma0_copy_words(*src, dst, 8);
        dst += 0x20;
        src++;
    } while (dst < 0x25F00320);
    sh2_dma0_copy_words(0x22400000, 0x25F003E0, 8);
    vdp2_apply_text_layer_commands(0x06075620);
    sh2_dma0_copy_words(0x22401020, 0x25E60000, 0x400);
    vdp2_pack_layer_addresses(3, 0x25E7C000, 0x25E7C000, 0x25E7C000, 0x25E7C000);
    g_06090294 = (void *)(uintptr_t)0x06026ecc;
    g_06090298 = (void *)(uintptr_t)0x06026ee2;
    g_0609029C = (void *)(uintptr_t)0x06026fe0;
    g_060902A0 = (void *)(uintptr_t)0x06027142;
    g_060997AC = 1;
}

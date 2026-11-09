/* scene_load_layer_bank2 [0x0220825c]: Loads 225dceb0/bc assets and initializes command block 06050b6c. */
#include "dh.h"
/* 0x0220825c - scene/layer loader (leaf, no args, no spawn). DMACs two ROM blobs (records at
 * 0x225dcebc / 0x225dceb0) into VDP regs (0x25f00200) and VRAM (0x25e3e000), zero-fills 0x400
 * cells at VDP1 VRAM 0x25e78000, emits a sprite/char block (func_0221675e), inits the
 * command-list struct 0x06050b6c (func_02220ade), and submits the draw (func_0222156c).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. func_0221675e(0x0f00, vram, 0x225dcec0, 0, 0,
 * 0x0200) = 4 register + 2 stacked args; DMA count = ((*0x225dceb0 << 5) >> 2). bsrf ->
 * 0x2145d0 x2, 0x213290, 0x21675e, 0x220ade, 0x22156c. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern s32  sh2_dmac_copy(void *src, void *dst, s32 count);  /* 0x2145d0 */
extern void memory_clear_vec4_blocks(void *dst, s32 count);          /* 0x213290 */
extern void tilemap_copy_wrap_wide(s32 a, void *b, void *c, s32 d, s32 e, s32 f);
extern void vdp2_apply_layer1_commands(void *arg);                         /* 0x220ade */
extern void vdp2_pack_layer_addresses(s32 a, void *b, void *c, void *d, void *e); /* 0x22156c */

void scene_load_layer_bank2(void) {
    void *vram = (void *)(uintptr_t)0x25e78000;

    sh2_dmac_copy(GP(0x225dcebc), (void *)(uintptr_t)0x25f00200, 8);
    sh2_dmac_copy(GP(0x225dceb4), (void *)(uintptr_t)0x25e3e000,
                  (G32(0x225dceb0) << 5) >> 2);
    memory_clear_vec4_blocks(vram, 0x400);
    tilemap_copy_wrap_wide(0x0f00, vram, (void *)(uintptr_t)0x225dcec0, 0, 0, 0x0200);
    vdp2_apply_layer1_commands((void *)(uintptr_t)0x06050b6c);
    vdp2_pack_layer_addresses(1, vram, vram, vram, vram);
}

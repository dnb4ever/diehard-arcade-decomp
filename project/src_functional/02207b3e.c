/* scene_load_layer_bank1 [0x02207b3e]: DMA records 226b1ddc/e8, clear plane and initialize command block 06050aa4. */
#include "dh.h"
/* 0x02207b3e - framebuffer/layer bring-up (leaf, no args). Two SH-2 DMAC ch0 copies of ROM
 * blobs described by records at 0x226b1ddc/0x226b1de8 into VDP registers (0x25f00200) and
 * VDP VRAM (0x25e38000), then zero-fills 0x400 vec4 cells at VDP1 VRAM 0x25e78000, inits the
 * command-list struct at 0x06050aa4 (func_02220ade), and submits a draw/setup for that VRAM
 * region (func_0222156c(1, vram, vram, vram, vram)).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. src pointers read from the ROM records;
 * second copy count is ((*0x226b1ddc << 5) >> 2) reproduced via the exact shift sequence;
 * func_0222156c takes 4 register args plus one stacked arg (all the VRAM base). bsrf ->
 * 0x2145d0 (x2), 0x213290, 0x220ade, 0x22156c. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern s32  sh2_dmac_copy(void *src, void *dst, s32 count);      /* 0x2145d0 */
extern void memory_clear_vec4_blocks(void *dst, s32 count);             /* 0x213290 */
extern void vdp2_apply_layer1_commands(void *arg);                            /* 0x220ade */
extern void vdp2_pack_layer_addresses(s32 a, void *b, void *c, void *d, void *e); /* 0x22156c */

void scene_load_layer_bank1(void) {
    void *vram = (void *)(uintptr_t)0x25e78000;

    sh2_dmac_copy(GP(0x226b1de8), (void *)(uintptr_t)0x25f00200, 8);
    sh2_dmac_copy(GP(0x226b1de0), (void *)(uintptr_t)0x25e38000,
                  (G32(0x226b1ddc) << 5) >> 2);
    memory_clear_vec4_blocks(vram, 0x400);
    vdp2_apply_layer1_commands((void *)(uintptr_t)0x06050aa4);
    vdp2_pack_layer_addresses(1, vram, vram, vram, vram);
}

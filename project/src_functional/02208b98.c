/* scene_load_perspective_strip_layer [0x02208b98]: Palette/plane DMA then child 02208c44; extent ends at tail spawn. */
#include "dh.h"
/* 0x02208b98 (scene_layer_loader_spawn_c) - layer bring-up task handler. Twin of
 * framebuffer_bringup (0x02207b3e) / scene_layer_loader (0x0220825c): DMAs two ROM blobs
 * into VDP CRAM/VRAM, fills a VDP1 VRAM region with a constant, inits a command-list
 * struct, programs the VDP2 plane, then tail-spawns a follow-up task handler.
 *
 * ABI: arg0 = task. Tail-calls func_0220ff54(task, func_02208c44) (the spawned handler
 * lives immediately after this function at SH-2 0x02208c44).
 *
 * IMPORTANT (DATA trap): m2c reports 0x208b98 as ~595 insns and fails to decompile it.
 * That is wrong - the real function is these ~7 calls and ends at the braf tail (cart
 * 0x208be6). What follows (cart 0x208bdc..0x208c43) is this function's literal pool, which
 * m2c mis-disassembled as code (the phantom `bt 0x208b0e`, the garbage `bsrf r0 ->
 * 0x5711c114`, and `bra 0x209656` all come from decoding pool longs as instructions).
 * The next real function is func_02208c44.
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm; every immediate read from cart.bin at the
 * pool addresses; bsrf targets from braf_resolve (0x2145d0 dmac x2, 0x2132ac fill,
 * 0x220ade cmdlist init, 0x22156c plane, tail 0x20ff54 spawn). */

extern s32   sh2_dmac_copy(void *src, void *dst, s32 count);               /* 0x2145d0 */
extern void  fill_vec4_array(void *base, s32 count, s32 value);            /* 0x2132ac */
extern void  vdp2_apply_layer1_commands(void *arg);                                      /* 0x220ade */
extern void  vdp2_pack_layer_addresses(s32 a, void *b, void *c, void *d, void *e);      /* 0x22156c */
extern void *task_spawn_current_work40(void *task, void *handler);                     /* 0x20ff54 spawn */
extern void  background_tick_perspective_strips(void *task);                                    /* spawned handler */

void scene_load_perspective_strip_layer(void *task)
{
    void *vram = (void *)(uintptr_t)0x25e78000;

    sh2_dmac_copy((void *)(uintptr_t)0x2240a520, (void *)(uintptr_t)0x25f00200, 8);
    sh2_dmac_copy((void *)(uintptr_t)0x22406020, (void *)(uintptr_t)0x25e38000, 0x1000);
    fill_vec4_array(vram, 0x400, 0x07000700);
    vdp2_apply_layer1_commands((void *)(uintptr_t)0x06050cc8);
    vdp2_pack_layer_addresses(1, vram, vram, vram, vram);
    task_spawn_current_work40(task, (void *)background_tick_perspective_strips);   /* tail call */
}

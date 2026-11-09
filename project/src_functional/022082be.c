/* scene_display_initialize [0x022082be]: Uploads scene display assets and spawns its per-frame display handler. */
#include "decomp.h"
/* 0x022082be (scene_display_init) - VDP1/VDP2 VRAM setup, then install the scene's
 * per-frame handler. Exposed as a distinct entry by fn.extent's over-swallow of the
 * 0x2082be..0x208988 pool group (this TU is only 0x2082be's own body, which ends at
 * the braf tail at 0x20832a; the big handler it installs is the separate function at
 * 0x022083a0, and a VDP transfer unit sits between them).
 *
 * ABI: handler(arg0). arg0 = r4 (task/scene context; the original spills it to the
 * stack and reloads it only to pass to the tail call).
 *   - DMA a ROM table (*0x02676078, uncached mirror 0x22676078) to VDP1 VRAM 0x25F00200.
 *   - DMA the ROM record *0x0267606C (ptr @+4, length (*0x0267606C << 5) >> 2) to
 *     VDP2 VRAM 0x25E38000.
 *   - clear/fill VDP2 VRAM 0x25E78000 (func_02213290(dst,0x400), func_0221675e, func_0222156c).
 *   - func_02220ade(0x06050BC0).
 *   - tail: func_0220ff54(arg0, scene_handler_2083a0) installs 0x022083a0 as the handler.
 *
 * All 33 computed branches braf-resolved to real TARGETs; pointer 0x022083a0 via
 * ptr_resolve (mova+add). Bounded 0x2082be..0x20832e (braf tail). No tst/shift traps.
 * fidelity: HIGH (the leaf transfer helpers' arg roles are as m2c decoded them). */

extern void sh2_dma0_copy_words(s32 src, s32 dst, s32 n);                 /* 0x022145d0 */
extern void memory_clear_vec4_blocks(s32 dst, s32 n);                          /* 0x02213290 */
extern void tilemap_copy_wrap_wide(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f);/* 0x0221675e */
extern void vdp2_apply_layer1_commands(s32 arg);                                 /* 0x02220ade */
extern void vdp2_pack_layer_addresses(s32 a, s32 b, s32 c, s32 d, s32 e);       /* 0x0222156c */
extern void task_spawn_current_work40(void *arg0, void *handler);               /* 0x0220ff54 */

void scene_display_init(void *arg0)
{
    sh2_dma0_copy_words(MEMS32(0x22676078), 0x25F00200, 8);
    sh2_dma0_copy_words(MEMS32(0x2267606C + 4), 0x25E38000, (MEMS32(0x2267606C) << 5) >> 2);
    memory_clear_vec4_blocks(0x25E78000, 0x400);
    tilemap_copy_wrap_wide(0xC00, 0x25E78000, 0x22676084, 0, 0, 0x200);
    vdp2_apply_layer1_commands(0x06050BC0);
    vdp2_pack_layer_addresses(1, 0x25E78000, 0x25E78000, 0x25E78000, 0x25E78000);
    task_spawn_current_work40(arg0, (void *)0x0600c3a0 /* PC-relative pointer in HWRAM */);
}

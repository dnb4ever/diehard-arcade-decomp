/* scene_load_line_scroll_layer [0x02208030]: Loads 226af5e4/f0 assets then spawns line_scroll_fx_sm with X=0x1600000. */
#include "dh.h"
/* 0x02208030 - scene/layer loader + object spawn. DMACs two ROM blobs (records at
 * 0x226af5f0 / 0x226af5e4) into VDP regs (0x25f00200) and VRAM (0x25e38000), zero-fills
 * 0x400 cells at VDP1 VRAM 0x25e78000, emits a sprite/char block (func_0221675e), inits the
 * command-list struct 0x06050b10 (func_02220ade), submits the draw (func_0222156c), then spawns
 * the follow-up task line_scroll_fx_sm via func_0220ff54(task, handler) and seeds the new object's
 * entity: (obj->unk8)->unk8 = 0x01600000.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. func_0221675e takes 4 register + 2 stacked args
 * (0x0c00, vram, 0x226af5f4, 30, 6, 0x0203); func_0220ff54 returns the spawned object in r0
 * (m2c mis-rendered this as the mova base); DMA count = ((*0x226af5e4 << 5) >> 2). bsrf ->
 * 0x2145d0 x2, 0x213290, 0x21675e, 0x220ade, 0x22156c, 0x20ff54. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern s32   sh2_dmac_copy(void *src, void *dst, s32 count);  /* 0x2145d0 */
extern void  memory_clear_vec4_blocks(void *dst, s32 count);          /* 0x213290 */
extern void  tilemap_copy_wrap_wide(s32 a, void *b, void *c, s32 d, s32 e, s32 f);
extern void  vdp2_apply_layer1_commands(void *arg);                         /* 0x220ade */
extern void  vdp2_pack_layer_addresses(s32 a, void *b, void *c, void *d, void *e); /* 0x22156c */
extern void *task_spawn_current_work40(void *parent, void *handler);      /* 0x20ff54 */
extern void  line_scroll_fx_sm(void *task);

void scene_load_line_scroll_layer(void *task) {
    void *vram = (void *)(uintptr_t)0x25e78000;
    void *obj;
    void *e;

    sh2_dmac_copy(GP(0x226af5f0), (void *)(uintptr_t)0x25f00200, 8);
    sh2_dmac_copy(GP(0x226af5e8), (void *)(uintptr_t)0x25e38000,
                  (G32(0x226af5e4) << 5) >> 2);
    memory_clear_vec4_blocks(vram, 0x400);
    tilemap_copy_wrap_wide(0x0c00, vram, (void *)(uintptr_t)0x226af5f4, 30, 6, 0x0203);
    vdp2_apply_layer1_commands((void *)(uintptr_t)0x06050b10);
    vdp2_pack_layer_addresses(1, vram, vram, vram, vram);

    obj = task_spawn_current_work40(task, (void *)line_scroll_fx_sm);
    e = MP(obj, 8);
    M32(e, 0x08) = 0x01600000;
}

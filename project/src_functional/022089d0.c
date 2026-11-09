/* scene_load_shutter_layer [0x022089d0]: Loads 2267606c/78 assets and spawns disp_shutter_sm with Z=0x50000. */
#include "dh.h"
/* 0x022089d0 - scene/layer loader + object spawn. DMACs two ROM blobs (records at
 * 0x22676078 / 0x2267606c) into VDP regs (0x25f00200) and VRAM (0x25e38000), zero-fills 0x400
 * cells at VDP1 VRAM 0x25e78000, emits a sprite/char block (func_0221675e), inits the
 * command-list struct 0x06050c64 (func_02220ade), submits the draw (func_0222156c), then spawns
 * the follow-up task disp_shutter_sm via func_0220ff54(task, handler) and seeds the new object's
 * entity: (obj->unk8)->unk10 = 0x50000, (obj->unk8)->unk32 = 0x20.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. func_0221675e(0x0c00, vram, 0x2267607c, 11, 6,
 * 0x0200); func_0220ff54 returns the spawned object in r0; DMA count = ((*0x2267606c<<5)>>2);
 * unk32 is a 16-bit field. bsrf -> 0x2145d0 x2, 0x213290, 0x21675e, 0x220ade, 0x22156c,
 * 0x20ff54. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern s32   sh2_dmac_copy(void *src, void *dst, s32 count);  /* 0x2145d0 */
extern void  memory_clear_vec4_blocks(void *dst, s32 count);          /* 0x213290 */
extern void  tilemap_copy_wrap_wide(s32 a, void *b, void *c, s32 d, s32 e, s32 f);
extern void  vdp2_apply_layer1_commands(void *arg);                         /* 0x220ade */
extern void  vdp2_pack_layer_addresses(s32 a, void *b, void *c, void *d, void *e); /* 0x22156c */
extern void *task_spawn_current_work40(void *parent, void *handler);      /* 0x20ff54 */
extern void  disp_shutter_sm(void *task);

void scene_load_shutter_layer(void *task) {
    void *vram = (void *)(uintptr_t)0x25e78000;
    void *obj;
    void *e;

    sh2_dmac_copy(GP(0x22676078), (void *)(uintptr_t)0x25f00200, 8);
    sh2_dmac_copy(GP(0x22676070), (void *)(uintptr_t)0x25e38000,
                  (G32(0x2267606c) << 5) >> 2);
    memory_clear_vec4_blocks(vram, 0x400);
    tilemap_copy_wrap_wide(0x0c00, vram, (void *)(uintptr_t)0x2267607c, 11, 6, 0x0200);
    vdp2_apply_layer1_commands((void *)(uintptr_t)0x06050c64);
    vdp2_pack_layer_addresses(1, vram, vram, vram, vram);

    obj = task_spawn_current_work40(task, (void *)disp_shutter_sm);
    e = MP(obj, 8);
    M32(e, 0x10) = 0x50000;
    M16(e, 0x32) = 0x20;
}

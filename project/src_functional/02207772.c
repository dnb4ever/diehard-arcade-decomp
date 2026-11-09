/* scene_load_streamed_layer [0x02207772]: DMA records 226edc04/0c, tile streaming and child 02207884. */
#include "dh.h"
/* 0x02207772 - scene/layer loader with a streaming loop + task spawn. Inits the command-list
 * struct 0x060509dc (func_02220828), DMACs two ROM blobs (records at 0x226edc0c / 0x226edc04)
 * into VDP regs (0x25f00600, count=*0x226edc0c>>1) and VRAM (0x25e20000, count=*0x226edc04<<4),
 * then loops the record table 0x226edc14 (stride 8) emitting sprite/char blocks via func_0221675e
 * into VRAM 0x25e70000.. stepping 0x2000 until 0x226edc24, submits the draw
 * (func_0222156c(0, 0x25e70000, 0x25e72000, 0x25e70000, 0x25e72000)), and spawns the follow-up
 * task func_02207884 via func_0220ff54(task, handler).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. func_0221675e called with (0, dst, &record, 0,0,0)
 * (4 register + 2 stacked args); loop pointer func_0221675e = mova 0x2077bc + 0xefa2 = 0x21675e;
 * loop is do/while (src < 0x226edc24, unsigned). bsrf -> 0x220828/0x2145d0 x2/0x22156c/0x20ff54. */
#define GU32(a) (*(u32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))

extern void vdp2_apply_layer0_commands(void *arg);
extern s32  sh2_dmac_copy(void *src, void *dst, s32 count);  /* 0x2145d0 */
extern void tilemap_copy_wrap_wide(s32 a, void *b, void *c, s32 d, s32 e, s32 f);
extern void vdp2_pack_layer_addresses(s32 a, void *b, void *c, void *d, void *e); /* 0x22156c */
extern void task_spawn_current_work40(void *parent, void *handler);      /* 0x20ff54 */
extern void background_tick_view_transform(void *task);

void scene_load_streamed_layer(void *task) {
    u8 *src;
    u32 dst;

    vdp2_apply_layer0_commands((void *)(uintptr_t)0x060509dc);
    sh2_dmac_copy(GP(0x226edc10), (void *)(uintptr_t)0x25f00600, (s32)(GU32(0x226edc0c) >> 1));
    sh2_dmac_copy(GP(0x226edc08), (void *)(uintptr_t)0x25e20000, (s32)(GU32(0x226edc04) << 4));

    src = (u8 *)(uintptr_t)0x226edc14;
    dst = 0x25e70000;
    do {
        tilemap_copy_wrap_wide(0, (void *)(uintptr_t)dst, src, 0, 0, 0);
        dst += 0x2000;
        src += 8;
        tilemap_copy_wrap_wide(0, (void *)(uintptr_t)dst, src, 0, 0, 0);
        src += 8;
        dst += 0x2000;
    } while ((u32)(uintptr_t)src < 0x226edc24);

    vdp2_pack_layer_addresses(0, (void *)(uintptr_t)0x25e70000, (void *)(uintptr_t)0x25e72000,
                 (void *)(uintptr_t)0x25e70000, (void *)(uintptr_t)0x25e72000);
    task_spawn_current_work40(task, (void *)background_tick_view_transform);
}

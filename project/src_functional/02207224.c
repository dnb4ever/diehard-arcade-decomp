/* scene_load_background_bank0 [0x02207224]: Scene setup caller; DMA palette/tiles and install layer continuation from 226d1fd4 records. */
#include "dh.h"
/* 0x02207224 - stage/background VRAM loader. Clears g_06098690, then DMACs ROM blobs into VDP
 * VRAM/registers: one into 0x25f00400 (count = *0x226d1fdc >> 1), one into 0x25e00000
 * (count = *0x226d1fd4 << 4), then a loop walking the record table at 0x226d1fe4 (stride 8,
 * +4 = source pointer) streaming 0x800-cell blocks into 0x25e40000.. stepping 0x2000 until
 * 0x25e44000. Then zero-fills 0x200 cells at 0x25e5c000, inits two palette/command structs
 * (func_022220be(0x06050898), func_022223c8(0x060508fc)), sets up two more layers
 * (func_02221f68(4, 0x25e5f000), func_02221bd4(0, 0x06050e48)), and finally installs this task's
 * continuation handler func_022072ce via func_0220ff54(task, handler).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. DMAC called through sh2_dmac_copy (mova base
 * 0x207240 + 0xd390 = 0x2145d0); loop is do/while (dst < 0x25e44000); handler 0x2072ce formed
 * by mova 0x2072b0 + 0x1e. bsrf -> 0x213290/0x2220be/0x2223c8/0x221f68/0x221bd4/0x20ff54. */
#define G32(a)   (*(s32 *)(uintptr_t)(a))
#define GU32(a)  (*(u32 *)(uintptr_t)(a))
#define GP(a)    (*(void **)(uintptr_t)(a))

extern s32  sh2_dmac_copy(void *src, void *dst, s32 count);  /* 0x2145d0 */
extern void memory_clear_vec4_blocks(void *dst, s32 count);          /* 0x213290 */
extern void vdp2_apply_rotation_commands(void *arg);
extern void vdp2_apply_colour_commands(void *arg);
extern void vdp2_setup_rotation_pair(s32 a, void *b);
extern void vdp2_pack_rotation_addresses(s32 a, void *b);
extern void task_spawn_current_work40(void *parent, void *handler);      /* 0x20ff54 */
extern void background_set_view_projection(void *task);

void scene_load_background_bank0(void *task) {
    u8 *tbl;
    u32 dst;

    G32(0x06098690) = 0;
    sh2_dmac_copy(GP(0x226d1fe0), (void *)(uintptr_t)0x25f00400, (s32)(GU32(0x226d1fdc) >> 1));
    sh2_dmac_copy(GP(0x226d1fd8), (void *)(uintptr_t)0x25e00000, (s32)(GU32(0x226d1fd4) << 4));

    tbl = (u8 *)(uintptr_t)0x226d1fe4;
    dst = 0x25e40000;
    do {
        sh2_dmac_copy(*(void **)(tbl + 4), (void *)(uintptr_t)dst, 0x800);
        tbl += 8;
        dst += 0x2000;
        sh2_dmac_copy(*(void **)(tbl + 4), (void *)(uintptr_t)dst, 0x800);
        dst += 0x2000;
        tbl += 8;
    } while (dst < 0x25e44000);

    memory_clear_vec4_blocks((void *)(uintptr_t)0x25e5c000, 0x200);
    vdp2_apply_rotation_commands((void *)(uintptr_t)0x06050898);
    vdp2_apply_colour_commands((void *)(uintptr_t)0x060508fc);
    vdp2_setup_rotation_pair(4, (void *)(uintptr_t)0x25e5f000);
    vdp2_pack_rotation_addresses(0, (void *)(uintptr_t)0x06050e48);
    task_spawn_current_work40(task, (void *)background_set_view_projection);
}

/* vdp2_setup_coefficient_pair [0x02222052]: Clears mode-six/seven buffers and registers their paired coefficient descriptors. */
#include "dh.h"
/* 0x02222052  mode 6/7 setup: clear a per-mode work buffer (handle at 0x060BAC98/0x060BAC9C)
 * then run builder func_0221636c against the matching pair of buffer bases.
 * Modes other than 6/7 do nothing.
 * fidelity: HIGH - matches asm; arg1 (r7) and arg2 (5th, stack-pushed) both reach the builder,
 * though m2c dropped the pushed 5th argument. */
extern s32 g_060BAC98;
extern s32 g_060BAC9C;
extern u8  g_060B8B98[];
extern u8  g_060B9118[];
extern u8  g_060B9C18[];
extern u8  g_060BA198[];
extern void memory_clear_vec4_blocks(s32 handle, s32 count);
extern void vdp2_register_attribute_pair(s32 mode, void *a, void *b, s32 arg1, s32 size);
void vdp2_setup_coefficient_pair(s32 mode, s32 arg1, s32 arg2) {
    switch (mode) {
    case 6:
        memory_clear_vec4_blocks(g_060BAC98, arg2 >> 2);
        vdp2_register_attribute_pair(6, g_060B8B98, g_060B9118, arg1, arg2);
        break;
    case 7:
        memory_clear_vec4_blocks(g_060BAC9C, arg2 >> 2);
        vdp2_register_attribute_pair(7, g_060B9C18, g_060BA198, arg1, arg2);
        break;
    }
}

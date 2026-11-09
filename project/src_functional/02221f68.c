/* vdp2_setup_rotation_pair [0x02221f68]: Resets the mode-four/five rotation block and registers paired transform buffers. */
#include "dh.h"
/* 0x02221f68  mode 4/5 setup: reset the mode's transform block (handle at 0x060B8B90/0x060B8B94)
 * via func_02221ffa, then run builder func_0221636c over the matching pair of buffer bases with
 * element size 0x1C. Sibling of func_02222052 (modes 6/7). Other modes do nothing.
 * fidelity: HIGH - matches asm; builder takes a 5th (stack) argument that m2c dropped. */
extern void *g_060B8B90;
extern void *g_060B8B94;
extern u8  g_060B88F0[];
extern u8  g_060B8960[];
extern u8  g_060B8A40[];
extern u8  g_060B8AB0[];
extern void vdp2_reset_transform_block(void *p);
extern void vdp2_register_attribute_pair(s32 mode, void *a, void *b, s32 arg1, s32 size);
void vdp2_setup_rotation_pair(s32 mode, s32 arg1) {
    switch (mode) {
    case 4:
        vdp2_reset_transform_block(g_060B8B90);
        vdp2_register_attribute_pair(4, g_060B88F0, g_060B8960, arg1, 0x1C);
        break;
    case 5:
        vdp2_reset_transform_block(g_060B8B94);
        vdp2_register_attribute_pair(5, g_060B8A40, g_060B8AB0, arg1, 0x1C);
        break;
    }
}

/* vdp2_clear_vram [0x022161c8]: Clears all 0x80000 bytes of VDP2 VRAM through the block zero helper. */
/* 0x022161c8  clear_vdp2_vram: tail-jump (braf) into func_02213290 to zero the
 * 0x8000 * 16 = 0x80000 bytes of VDP2 VRAM at 0x25e00000.
 * (This is a PIC thunk; the adjacent code at 0x2161d2/0x2161dc are separate
 * entry points - a VDP1 fb clear thunk and func_022161dc.)
 * fidelity: faithful - braf r3 resolved to func_02213290, args r4/r5. */
#include "dh_band21.h"

extern void memory_clear_vec4_blocks(s32 base, s32 count);

void vdp2_clear_vram(void) {
    memory_clear_vec4_blocks(0x25e00000, 0x8000);
}

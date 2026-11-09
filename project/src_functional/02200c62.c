/* 0x02200c62  scene_flags_reset_b: byte-identical sibling of scene_flags_reset (0x2005fa).
 * Same HWRAM flag group, same pool (a second copy at 0x200c94); a distinct boot/init path
 * emits the same reset sequence.
 *   g_06020628 = 0;  g_0602e570 = 1;  g_0602cdb4 = 0;  g_0602cdb8 = 0;
 *   g_0603e574 = 1;  g_06025320 = 0;
 *   *(s32*)(0x0600e56c + 0x108) = 0;  *(s32*)(0x0600e56c + 0x104) = 0;   (last in rts delay)
 * fidelity: HIGH - identical 2-aligned-pool halfword-swap case as 0x2005fa (see pool_read.py). */
#include "decomp.h"

void scene_flags_reset_b(void) {
    MEM32(0x06020628) = 0;
    MEM32(0x0602e570) = 1;
    MEM32(0x0602cdb4) = 0;
    MEM32(0x0602cdb8) = 0;
    MEM32(0x0603e574) = 1;
    MEM32(0x06025320) = 0;
    MEM32(0x0600e56c + 0x108) = 0;
    MEM32(0x0600e56c + 0x104) = 0;
}

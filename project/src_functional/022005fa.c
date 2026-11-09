/* 0x022005fa  scene_flags_reset: zero a group of HWRAM render/scene state flags and set
 * two of them to 1 (boot/init leaf).
 *   g_06020628 = 0;  g_0602e570 = 1;  g_0602cdb4 = 0;  g_0602cdb8 = 0;
 *   g_0603e574 = 1;  g_06025320 = 0;
 *   *(s32*)(0x0600e56c + 0x108) = 0;  *(s32*)(0x0600e56c + 0x104) = 0;   (last in rts delay)
 * fidelity: HIGH - this boot-block literal pool is 2-aligned, so its mov.l longwords read
 *   back with their 16-bit halves swapped (raw 0xe5700602 == real 0x0602e570); the real
 *   constants are recovered via tools/decomp/pool_read.py and are ALL valid HWRAM (three of
 *   them - 0x0602cdb4/cdb8/e570 - also occur as clean 4-aligned constants elsewhere in ROM,
 *   confirming the swap). The two mov.w offsets (0x104/0x108) read cleanly. */
#include "decomp.h"

void scene_flags_reset(void) {
    MEM32(0x06020628) = 0;
    MEM32(0x0602e570) = 1;
    MEM32(0x0602cdb4) = 0;
    MEM32(0x0602cdb8) = 0;
    MEM32(0x0603e574) = 1;
    MEM32(0x06025320) = 0;
    MEM32(0x0600e56c + 0x108) = 0;
    MEM32(0x0600e56c + 0x104) = 0;
}

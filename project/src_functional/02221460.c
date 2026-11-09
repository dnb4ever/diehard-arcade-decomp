/* vdp2_set_transform_slot0 [0x02221460]: Divide-derived weights and origin corrections written to shadow+70..7c. */
#include "dh.h"
/* 0x02221460  configure interpolated transform slot 0 on the display object *0x06098F08.
 * For each of the two axes a weight w = sh2_hw_divide(0x10000, step) is computed and stored
 * (0x78 / 0x7C); the matching position field (0x70 / 0x74) is set to (0x10000 - w) * base +
 * (origin << 16). Twin of func_022214d2, which drives slot 1 (fields 0x80..0x8C).
 * fidelity: HIGH - the ROM saves MACL, so m2c's "arg5/arg6" are the real 5th/6th stack args. */
extern void *g_vdp2_register_shadow;
extern s32 sh2_hw_divide(s32 a, s32 b);
void vdp2_set_transform_slot0(s16 base0, s16 base1, s32 origin0, s32 origin1, s32 step0, s32 step1) {
    void *o = g_vdp2_register_shadow;
    s32 w0 = sh2_hw_divide(0x10000, step0);
    s32 w1 = sh2_hw_divide(0x10000, step1);
    M32(o, 0x78) = w0;
    M32(o, 0x7C) = w1;
    M32(o, 0x70) = (0x10000 - w0) * base0 + (origin0 << 16);
    M32(o, 0x74) = (0x10000 - w1) * base1 + (origin1 << 16);
}

/* vdp2_configure_layer1_interpolation [0x022214d2]: Calculates reciprocal steps and initializes the second background's scroll and zoom fields. */
#include "dh.h"
/* 0x022214d2  configure interpolated transform slot 1 on the display object *0x06098F08.
 * Twin of func_02221460 (slot 0); here the weights land at 0x88 / 0x8C and the position
 * fields at 0x80 / 0x84.
 * fidelity: HIGH - the ROM saves MACL, so m2c's "arg5/arg6" are the real 5th/6th stack args. */
extern void *g_vdp2_register_shadow;
extern s32 sh2_hw_divide(s32 a, s32 b);
void vdp2_configure_layer1_interpolation(s16 base0, s16 base1, s32 origin0, s32 origin1, s32 step0, s32 step1) {
    void *o = g_vdp2_register_shadow;
    s32 w0 = sh2_hw_divide(0x10000, step0);
    s32 w1 = sh2_hw_divide(0x10000, step1);
    M32(o, 0x88) = w0;
    M32(o, 0x8C) = w1;
    M32(o, 0x80) = (0x10000 - w0) * base0 + (origin0 << 16);
    M32(o, 0x84) = (0x10000 - w1) * base1 + (origin1 << 16);
}

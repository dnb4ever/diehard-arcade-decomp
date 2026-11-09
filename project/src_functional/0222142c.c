/* vdp2_set_layer_zoom [0x0222142c]: Writes the selected background layer's two thirty-two-bit zoom fields. */
#include "dh.h"
/* 0x0222142c  store (a,b) into global object *0x06098F08 at a slot chosen by selector byte 0x060B3D50.
 * fidelity: HIGH - sel 0 -> +0x78/+0x7C; sel 1 -> +0x88/+0x8C (32-bit). */
extern void *g_vdp2_register_shadow;
extern s8    g_060B3D50;
void vdp2_set_layer_zoom(s32 a, s32 b) {
    void *o = g_vdp2_register_shadow;
    s8 sel = g_060B3D50;
    if (sel == 0) {
        M32(o, 0x78) = a;
        M32(g_vdp2_register_shadow, 0x7C) = b;
    } else if (sel == 1) {
        M32(o, 0x88) = a;
        M32(g_vdp2_register_shadow, 0x8C) = b;
    }
}

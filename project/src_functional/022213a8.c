/* vdp2_set_layer_scroll [0x022213a8]: Writes the selected layer's horizontal and vertical scroll fields. */
#include "dh.h"
/* 0x022213a8  store (a,b) into global object *0x06098F08, slot chosen by selector byte 0x060B3D50.
 * fidelity: HIGH - sel0 +0x70/74 (32b), sel1 +0x80/84 (32b), sel2 +0x90/92 (16b), sel3 +0x94/96 (16b). */
extern void *g_vdp2_register_shadow;
extern s8    g_060B3D50;
void vdp2_set_layer_scroll(s32 a, s32 b) {
    void *o = g_vdp2_register_shadow;
    s8 sel = g_060B3D50;
    switch (sel) {
    case 0:
        M32(o, 0x70) = a;
        M32(g_vdp2_register_shadow, 0x74) = b;
        break;
    case 1:
        M32(o, 0x80) = a;
        M32(g_vdp2_register_shadow, 0x84) = b;
        break;
    case 2:
        M16(o, 0x90) = (s16)a;
        M16(g_vdp2_register_shadow, 0x92) = (s16)b;
        break;
    case 3:
        M16(o, 0x94) = (s16)a;
        M16(g_vdp2_register_shadow, 0x96) = (s16)b;
        break;
    }
}

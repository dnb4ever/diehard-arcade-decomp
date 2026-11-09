/* view_initialize_transform [0x02222574]: Initializes the selected display transform record and its mode/busy bookkeeping. */
#include "dh.h"
/* 0x02222574  init a transform/display object (one of two, selected by `which`); common field fill.
 * fidelity: HIGH - 0x34..0x40 are 16-bit (mov.w), 0x0..0x5C are 32-bit (mov.l). */
extern void *g_060B8B90;
extern void *g_060B8B94;
extern s32   g_060BACA0;
extern s32   g_060BACA4;
extern s32   g_060BACAC;
extern s32   g_060BACB0;
void view_initialize_transform(s32 which, s32 arg1, s32 arg2) {
    void *o;
    if (which == 1) {
        o = g_060B8B94;
        g_060BACA4 = arg1;
        if (arg2 != 0) {
            g_060BACB0 |= 2;
        }
    } else {
        o = g_060B8B90;
        g_060BACA0 = arg1;
        if (arg2 != 0) {
            g_060BACAC |= 2;
        }
    }
    M16(o, 0x34) = 0xB0;
    M16(o, 0x3C) = 0xB0;
    M16(o, 0x36) = 0x78;
    M16(o, 0x3E) = 0x78;
    M16(o, 0x38) = 0x100;
    M16(o, 0x40) = 0x100;
    M32(o, 0x0)  = 0;
    M32(o, 0x4)  = 0;
    M32(o, 0x8)  = 0;
    M32(o, 0xC)  = 0;
    M32(o, 0x10) = 0x10000;
    M32(o, 0x14) = 0x10000;
    M32(o, 0x18) = 0;
    M32(o, 0x58) = 0x10000;
    M32(o, 0x5C) = 0;
}

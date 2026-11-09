/* vdp2_set_plane_nibble [0x02221800]: Converts address and inserts three-bit value into selected shadow+3c nibble. */
#include "dh.h"
/* 0x02221800  set a 4-bit field in the 16-bit word at off 0x3C of *0x06098F08.
 * v = (((arg1 + 0xDA200000) >> 1) >> 16) & 7; which==0 -> low nibble, which==1 -> next nibble.
 * fidelity: HIGH - exact (shar=arithmetic >>1, shlr16=logical >>16, exts.w, and #7). */
extern void *g_vdp2_register_shadow;
void vdp2_set_plane_nibble(s32 which, s32 arg1) {
    void *o = g_vdp2_register_shadow;
    s16 v = (s16)(((u32)((s32)(arg1 + 0xDA200000) >> 1) >> 16) & 7);
    s16 f = M16(o, 0x3C);
    if (which == 0) {
        M16(o, 0x3C) = (s16)((f & ~0xF) | v);
    } else if (which == 1) {
        M16(o, 0x3C) = (s16)((f & ~0xF0) | (v << 4));
    }
}

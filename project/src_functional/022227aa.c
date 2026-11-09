/* display_set_priority_origin [0x022227aa]: Rebases address to priority word and hardware halfword, sets shadow+a8. */
#include "dh.h"
/* 0x022227aa  v = (u32)(arg0 + 0xDA100000) >> 1; store to 0x060BACA8 (32b) and HW reg 0x25E5FFFE (16b);
 * set *0x06098F08 field 0xA8 = 0x12F2FFFF.
 * fidelity: HIGH - exact. */
extern void *g_vdp2_register_shadow;
extern u32   g_060BACA8;
extern volatile s16 reg_25E5FFFE;
void display_set_priority_origin(s32 arg0) {
    u32 v = (u32)(arg0 + 0xDA100000) >> 1;
    g_060BACA8 = v;
    reg_25E5FFFE = (s16)v;
    M32(g_vdp2_register_shadow, 0xA8) = 0x12F2FFFF;
}

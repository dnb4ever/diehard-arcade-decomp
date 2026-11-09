/* vdp2_enable_shadow_bit5 [0x022223b2]: Sets bit 0x20 in shadow halfword+20. */
#include "dh.h"
/* 0x022223b2  set bit 0x20 in the 16-bit field at off 0x20 of the global object *0x06098F08.
 * fidelity: HIGH - exact ((x & ~0x20) | 0x20). */
extern void *g_vdp2_register_shadow;
void vdp2_enable_shadow_bit5(void) {
    void *o = g_vdp2_register_shadow;
    M16(o, 0x20) = (s16)((M16(o, 0x20) & ~0x20) | 0x20);
}

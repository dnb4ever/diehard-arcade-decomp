/* vdp2_disable_layer_bit [0x02221384]: Clears the selected bit of the background-enable shadow word. */
#include "dh.h"
/* 0x02221384  vdp2_shadow_clrbit  (clear a bit in a VDP2 register-shadow field).
 *
 * Sibling of vdp2_shadow_setbit (0x02221354); previously deferred for the same
 * reason (non-standard r0/r1/r2 bsrf into the 0x24a360 shift-left helper).  Calls
 * func_0224a360(1, bit) == (1 << bit) once, then clears that bit in the 16-bit
 * field at offset 0x20 of the VDP2 shadow block (*g_06098F08) and stores it back.
 *
 * fidelity: HIGH - control flow + 16-bit width from the mov.w load/store at offset
 * 0x20; bsrf target confirmed by braf_resolve.py (delta 0x28fcc -> 0x24a360). */
extern void *g_vdp2_register_shadow;                           /* -> VDP2 register shadow block */
extern s32 shc_shift_left(s32 value, s32 count);    /* variable shift-left: value << count */

void vdp2_disable_layer_bit(s32 bit) {
    void *p = g_vdp2_register_shadow;
    M16(p, 0x20) = (s16)(M16(p, 0x20) & ~shc_shift_left(1, bit));
}

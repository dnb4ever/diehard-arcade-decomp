/* vdp2_shadow_setbit [0x02221354]: Existing map corroborated by source: void(s32 bit r4): set bit "bit" in M16(*g_06098F08, 0x20). Calls ashl_var(1,bit) twice via bsrf (deltas 0x28ffa/0x28fe6). r0=value in bsrf delay slot. */
#include "dh.h"
/* 0x02221354  vdp2_shadow_setbit  (set a bit in a VDP2 register-shadow field).
 *
 * Previously deferred: it calls the 0x24a360 variable shift-left helper twice via
 * the non-standard r0/r1/r2 ABI (bsrf -> 0x24a360, value=1 placed in r0 in the
 * delay slot, count in r1).  Now decodable: func_0224a360(1, bit) == (1 << bit).
 *
 * Reads the 16-bit field at offset 0x20 of the VDP2 shadow block (*g_06098F08),
 * forces its bit "bit" to 1 (clear-then-set, as the compiler emitted it - two
 * separate 1<<bit evaluations because the shift is an opaque call that blocks CSE),
 * and stores it back.  Net effect: set bit "bit".
 *
 * fidelity: HIGH - control flow, 16-bit width (mov.w, sign-extended load / low-16
 * store at offset 0x20) and the two shift-helper calls all read straight from ROM;
 * bsrf targets confirmed by braf_resolve.py (delta 0x28ffa / 0x28fe6 -> 0x24a360). */
extern void *g_vdp2_register_shadow;                           /* -> VDP2 register shadow block */
extern s32 shc_shift_left(s32 value, s32 count);    /* variable shift-left: value << count */

void vdp2_shadow_setbit(s32 bit) {
    void *p = g_vdp2_register_shadow;
    M16(p, 0x20) = (s16)((M16(p, 0x20) & ~shc_shift_left(1, bit)) | shc_shift_left(1, bit));
}

/* vdp2_pack_rotation_addresses [0x02221bd4]: Packs an array of biased address components into the selected rotation shadow bank. */
#include "dh.h"
/* 0x02221bd4  vdp2_shadow_pack6_arr  (pack 6-bit component pairs into the VDP2
 * register-shadow matrix block - array-argument entry).
 *
 * Previously deferred: the body is 33 calls to the 0x24a420 variable
 * arithmetic-shift-right helper (ashr_var) via the non-standard r0/r1 bsrf ABI
 * (value+bias in r0, count in r1).  braf_resolve.py confirms every bsrf delta
 * resolves to 0x24a420, so each call is func_0224a420(value, count).
 *
 * Reads the VDP2 shadow block pointer from *g_06098F08 (r7), picks a shift
 * "mode" (11/12/13/14) from two shadow-state bits, then writes a bank of nine
 * 16-bit shadow fields:
 *   - field 0x3E: low nibble (which==0) or bits 4-7 cleared (which==1, as the
 *     compiler emitted it) OR'd with a 3-bit code derived from arg1[0].
 *   - fields base..base+0xE (base = 0x50 for which==0, 0x60 for which==1):
 *     eight 16-bit words, each packing two 6-bit components
 *       (comp(arg1[2i+1]) << 8) | comp(arg1[2i])
 *     where comp(v) = (ashr_var(v + 0xDA200000, mode) & 0x3F).
 * which not in {0,1}: nothing is written.
 *
 * Every shift count is in 11..20 (mode / mode+6), so the helper's count<0 and
 * count>=32 edge cases never trigger here; ashr_var(v,n) is a plain arithmetic
 * (v >> n).  The helper is called for faithfulness (matches the ROM one-to-one).
 *
 * fidelity: HIGH - control flow, the mode selector, the 0xDA200000 bias, the
 * 6-bit packing and all 33 shift calls read straight from ROM asm; bsrf targets
 * confirmed by braf_resolve.py (all -> 0x24a420). */
extern void *g_vdp2_register_shadow;                         /* -> VDP2 register-shadow block */
extern s32 shc_shift_right_signed(s32 value, s32 count);  /* ashr_var: (signed)value >> count */

/* one packed 6-bit component: arithmetic-shift the biased source right, keep 6 bits */
static s32 comp6(s32 v, s32 count) {
    return shc_shift_right_signed((s32)((u32)v + 0xDA200000u), count) & 0x3F;
}

void vdp2_pack_rotation_addresses(s32 which, void *arg1) {
    void *vdp = g_vdp2_register_shadow;
    s32 mode;
    s32 code;
    s32 field3e;
    s32 base;
    int i;

    if (M16(vdp, 0x38) & 0x8000)
        mode = (M16(vdp, 0x2A) & 0x100) ? 11 : 13;
    else
        mode = (M16(vdp, 0x2A) & 0x100) ? 12 : 14;

    code = shc_shift_right_signed((s32)((u32)M32(arg1, 0) + 0xDA200000u), mode + 6) & 7;

    field3e = M16(vdp, 0x3E);
    if (which == 0) {
        M16(vdp, 0x3E) = (s16)((field3e & ~0xF) | code);
        base = 0x50;
    } else if (which == 1) {
        M16(vdp, 0x3E) = (s16)((field3e & ~0xF0) | code);
        base = 0x60;
    } else {
        return;
    }

    for (i = 0; i < 8; i++)
        M16(vdp, base + 2 * i) =
            (s16)((comp6(M32(arg1, 8 * i + 4), mode) << 8) | comp6(M32(arg1, 8 * i), mode));
}

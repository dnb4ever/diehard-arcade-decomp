/* vdp2_shadow_pack6_args [0x0222184c]: Existing map corroborated by source: void(s32 which, s32 v0..v15). Same packer as 0x2221bd4 but 16 scalar args (v0..v2 in r5-r7, v3..v15 on stack) instead of an array. 33 ashr_var. real entry 0x22156c-style find_entry fix not needed (0x22184c is the corrected entry for mission's 0x221842). Hand-verified. */
#include "dh.h"
/* 0x0222184c  vdp2_shadow_pack6_args  (pack 6-bit component pairs into the VDP2
 * register-shadow matrix block - scalar-argument entry).
 *
 * Same VDP2 shadow packer as func_02221bd4, but the sixteen source components
 * arrive as individual arguments instead of an array: v0..v2 in r5/r6/r7 and
 * v3..v15 on the caller stack.  Previously deferred: 33 bsrf calls to the
 * 0x24a420 arithmetic-shift-right helper (ashr_var) via the non-standard r0/r1
 * ABI; braf_resolve.py confirms every delta resolves to 0x24a420.
 *
 * Reads the VDP2 shadow block pointer from *g_06098F08, picks a shift "mode"
 * (11/12/13/14) from two shadow-state bits, then writes:
 *   - field 0x3E: low nibble (which==0) / bits 4-7 cleared (which==1, as emitted)
 *     OR'd with a 3-bit code derived from v0.
 *   - fields base..base+0xE (base = 0x50 for which==0, 0x60 for which==1): eight
 *     16-bit words, each (comp(v[2i+1]) << 8) | comp(v[2i]) with
 *     comp(v) = ashr_var(v + 0xDA200000, mode) & 0x3F.
 * which not in {0,1}: nothing is written.
 *
 * All shift counts are 11..20 (mode / mode+6), so ashr_var is a plain arithmetic
 * right shift here; the helper is called for faithfulness.
 *
 * fidelity: HIGH - control flow, mode selector, 0xDA200000 bias, 6-bit packing
 * and all 33 shift calls read from ROM asm; bsrf targets confirmed (-> 0x24a420).
 * Argument marshalling (regs vs stack) is an ABI detail; the C models the values
 * and their use order faithfully. */
extern void *g_vdp2_register_shadow;                         /* -> VDP2 register-shadow block */
extern s32 shc_shift_right_signed(s32 value, s32 count);  /* ashr_var: (signed)value >> count */

static s32 comp6(s32 v, s32 count) {
    return shc_shift_right_signed((s32)((u32)v + 0xDA200000u), count) & 0x3F;
}

void vdp2_shadow_pack6_args(s32 which, s32 v0, s32 v1, s32 v2, s32 v3, s32 v4, s32 v5,
                   s32 v6, s32 v7, s32 v8, s32 v9, s32 v10, s32 v11, s32 v12,
                   s32 v13, s32 v14, s32 v15) {
    void *vdp = g_vdp2_register_shadow;
    s32 v[16];
    s32 mode;
    s32 code;
    s32 field3e;
    s32 base;
    int i;

    v[0] = v0;   v[1] = v1;   v[2] = v2;   v[3] = v3;
    v[4] = v4;   v[5] = v5;   v[6] = v6;   v[7] = v7;
    v[8] = v8;   v[9] = v9;   v[10] = v10; v[11] = v11;
    v[12] = v12; v[13] = v13; v[14] = v14; v[15] = v15;

    if (M16(vdp, 0x38) & 0x8000)
        mode = (M16(vdp, 0x2A) & 0x100) ? 11 : 13;
    else
        mode = (M16(vdp, 0x2A) & 0x100) ? 12 : 14;

    code = shc_shift_right_signed((s32)((u32)v[0] + 0xDA200000u), mode + 6) & 7;

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
            (s16)((comp6(v[2 * i + 1], mode) << 8) | comp6(v[2 * i], mode));
}

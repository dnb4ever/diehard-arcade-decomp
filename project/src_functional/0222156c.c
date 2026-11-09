/* vdp2_pack_layer_addresses [0x0222156c]: Packs four biased address components into the selected VDP2 shadow bank. */
#include "dh.h"
/* 0x0022156c  vdp2_shadow_pack4_sel  (pack four 6-bit components into a selected
 * bank of the VDP2 register-shadow matrix block).
 *
 * Previously deferred: 17 calls to the 0x24a420 arithmetic-shift-right helper
 * (ashr_var) via the non-standard r0/r1 bsrf ABI; braf_resolve.py confirms all
 * resolve to 0x24a420.  (Note: the real entry is 0x22156c; 0x22154e points a few
 * words into the previous function's trailing pool - find_entry.py.)
 *
 * Same packer family as func_02221bd4, but with a 4-way bank selector `sel` and
 * four scalar component arguments.  vdp = *g_06098F08.  `sel` (0..3) chooses both
 * the pair of shadow-state bits that pick the shift "mode" (11/12/13/14) and the
 * destination bank:
 *   sel 0: flag=bit0 of 0x28, sign=bit15 of 0x30, bank 0x40/0x42, nibble bits 0-3
 *   sel 1: flag=bit8 of 0x28, sign=bit15 of 0x32, bank 0x44/0x46, nibble bits 4-7
 *   sel 2: flag=bit0 of 0x2A, sign=bit15 of 0x34, bank 0x48/0x4A, nibble 8-11
 *   sel 3: flag=bit4 of 0x2A, sign=bit15 of 0x36, bank 0x4C/0x4E, nibble 12-15
 * then writes (base = 0x40 + 4*sel, shift = 4*sel):
 *   field 0x3C: nibble bits [shift..shift+3] <- 3-bit code from a1
 *   field base   : (comp(a2) << 8) | comp(a1)
 *   field base+2 : (comp(a4) << 8) | comp(a3)
 * with comp(v) = ashr_var(v + 0xDA200000, mode) & 0x3F and
 *      code     = ashr_var(a1 + 0xDA200000, mode + 6) & 7.
 * sel outside 0..3: the ROM still runs the (side-effect-free) code shift but
 * writes nothing, so this is a no-op.
 *
 * All shift counts are 11..20 (mode / mode+6), so ashr_var is a plain arithmetic
 * right shift here; the helper is called for faithfulness.
 *
 * fidelity: HIGH - the selector demux, mode logic, 0xDA200000 bias, nibble
 * placement, 6-bit packing and all shift calls read from ROM asm; shift targets
 * confirmed by braf_resolve.py (-> 0x24a420). */
extern void *g_vdp2_register_shadow;                          /* -> VDP2 register-shadow block */
extern s32 shc_shift_right_signed(s32 value, s32 count);   /* ashr_var: (signed)value >> count */

static s32 comp6(s32 v, s32 count) {
    return shc_shift_right_signed((s32)((u32)v + 0xDA200000u), count) & 0x3F;
}

void vdp2_pack_layer_addresses(s32 sel, s32 a1, s32 a2, s32 a3, s32 a4) {
    void *vdp = g_vdp2_register_shadow;
    s32 flag;
    s32 sign;
    s32 mode;
    s32 code;
    s32 shift;
    s32 base;

    switch (sel) {
    case 0: flag = M16(vdp, 0x28) & 0x001; sign = M16(vdp, 0x30) & 0x8000; break;
    case 1: flag = M16(vdp, 0x28) & 0x100; sign = M16(vdp, 0x32) & 0x8000; break;
    case 2: flag = M16(vdp, 0x2A) & 0x001; sign = M16(vdp, 0x34) & 0x8000; break;
    case 3: flag = M16(vdp, 0x2A) & 0x010; sign = M16(vdp, 0x36) & 0x8000; break;
    default: return;
    }

    if (sign)
        mode = flag ? 11 : 13;
    else
        mode = flag ? 12 : 14;

    code = shc_shift_right_signed((s32)((u32)a1 + 0xDA200000u), mode + 6) & 7;

    shift = 4 * sel;
    base = 0x40 + shift;
    M16(vdp, 0x3C) = (s16)((M16(vdp, 0x3C) & ~(0xF << shift)) | (code << shift));
    M16(vdp, base)     = (s16)((comp6(a2, mode) << 8) | comp6(a1, mode));
    M16(vdp, base + 2) = (s16)((comp6(a4, mode) << 8) | comp6(a3, mode));
}

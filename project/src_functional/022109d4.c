/* animation_sample_absolute [0x022109d4]: Advances the slot keyframe countdown and samples absolute position and angular outputs. */
/* 0x022109d4 (anim_channel_interp_abs) - advance and sample the keyframe animation
 * for channel ch = arg0->unk34. Identical in structure to func_02210876 (same
 * per-channel keyframe list 0x060e1218, countdown 0x060e1298, duration table
 * 0x0604ebe4, 0x18-byte keyframes), differing only in the three 32-bit outputs:
 * here arg0->unk14/18/1C take interp(cur->unk4/8/C, cur->unk2) (the absolute keyframe
 * value, not the cur-prev delta).
 *   - no list -> return -1; non-zero first halfword -> return it.
 *   - decrement countdown[ch]; on underflow advance keyframe and reload
 *     countdown[ch] = dur_table[cur->unk2] - 1.
 *   - 16-bit outputs arg0->unk24/26/28 = cur->unk10/12/14 -
 *     interp((cur - prev)*countdown[ch], cur->unk2).
 *   - return cur->unk0 if countdown[ch] != 0 else cur->unk18.
 * fidelity: faithful to operations - keyframe stride/offsets/widths verified. interp
 *   = func_0224a420, an out-of-band fixed-point helper taking args in r0/r1 (non-C
 *   register convention), modelled as a normal 2-arg call (functional, not byte-exact). */
#include "dh_band21.h"

extern s32 shc_shift_right_signed(s32 a, s32 b);

s16 animation_sample_absolute(u32 arg0) {
    s32 ch = MEM8(arg0 + 0x34);
    u32 kf = MEM32(0x060e1218 + ch * 4);
    s16 first;
    s32 cd;
    u32 cur, prev;

    if (kf == 0) return -1;
    first = MEM16(kf);
    if (first != 0) return first;

    cd = MEM32(0x060e1298 + ch * 4) - 1;
    MEM32(0x060e1298 + ch * 4) = cd;
    if (cd < 0) {
        cur = MEM32(0x060e1218 + ch * 4) + 0x18;
        MEM32(0x060e1218 + ch * 4) = cur;
        prev = cur - 0x18;
        MEM32(0x060e1298 + ch * 4) = MEM32(DH_ACTOR_SLOT_MASKS_ADDR + MEM16(cur + 2) * 4) - 1;
    } else {
        cur = MEM32(0x060e1218 + ch * 4);
        prev = cur - 0x18;
    }

    MEM32(arg0 + 0x14) = shc_shift_right_signed(MEM32(cur + 0x4), MEM16(cur + 2));
    MEM32(arg0 + 0x18) = shc_shift_right_signed(MEM32(cur + 0x8), MEM16(cur + 2));
    MEM32(arg0 + 0x1c) = shc_shift_right_signed(MEM32(cur + 0xc), MEM16(cur + 2));

    MEM16(arg0 + 0x24) = (s16)(MEM16(cur + 0x10) -
        shc_shift_right_signed((MEM16(cur + 0x10) - MEM16(prev + 0x10)) * MEM32(0x060e1298 + ch * 4), MEM16(cur + 2)));
    MEM16(arg0 + 0x26) = (s16)(MEM16(cur + 0x12) -
        shc_shift_right_signed((MEM16(cur + 0x12) - MEM16(prev + 0x12)) * MEM32(0x060e1298 + ch * 4), MEM16(cur + 2)));
    MEM16(arg0 + 0x28) = (s16)(MEM16(cur + 0x14) -
        shc_shift_right_signed((MEM16(cur + 0x14) - MEM16(prev + 0x14)) * MEM32(0x060e1298 + ch * 4), MEM16(cur + 2)));

    if (MEM32(0x060e1298 + ch * 4) == 0) return MEM16(cur + 0x18);
    return MEM16(cur);
}

/* animation_sample_channel [0x02210876]: Slot+34 selects 0x18-byte keyframes and per-channel countdown/interpolation state. */
/* 0x02210876 (anim_channel_interp) - advance and sample the keyframe animation for
 * channel ch = arg0->unk34. The per-channel keyframe list head is table[ch] at
 * 0x060e1218 (keyframes are 0x18 bytes); the per-channel countdown is at 0x060e1298;
 * segment durations are table 0x0604ebe4 (indexed by keyframe->unk2).
 *   - no list -> return -1; a non-zero first halfword of the list -> return it.
 *   - decrement countdown[ch]; when it goes negative, advance to the next keyframe
 *     (head += 0x18) and reload countdown[ch] = dur_table[cur->unk2] - 1.
 *   - write the per-frame 32-bit steps arg0->unk14/18/1C = interp(cur->unk4/8/C -
 *     prev->..., cur->unk2) and the 16-bit positions arg0->unk24/26/28 = cur->unk10/
 *     12/14 - interp((cur - prev)*countdown[ch], cur->unk2).
 *   - return cur->unk0 if countdown[ch] != 0, else cur->unk18 (segment-end marker).
 * fidelity: faithful to operations - keyframe stride 0x18, field offsets and widths
 *   (32-bit unk4/8/C, s16 unk2/10/12/14), countdown reload. interp = func_0224a420,
 *   an out-of-band fixed-point helper whose args arrive in r0/r1 (non-C register
 *   convention) and result in r0; modelled here as a normal 2-arg call (functional
 *   decomp, not byte-exact) - its internal scaling is not reproduced here. */
#include "dh_band21.h"

extern s32 shc_shift_right_signed(s32 a, s32 b);

s16 animation_sample_channel(u32 arg0) {
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

    MEM32(arg0 + 0x14) = shc_shift_right_signed(MEM32(cur + 0x4) - MEM32(prev + 0x4), MEM16(cur + 2));
    MEM32(arg0 + 0x18) = shc_shift_right_signed(MEM32(cur + 0x8) - MEM32(prev + 0x8), MEM16(cur + 2));
    MEM32(arg0 + 0x1c) = shc_shift_right_signed(MEM32(cur + 0xc) - MEM32(prev + 0xc), MEM16(cur + 2));

    MEM16(arg0 + 0x24) = (s16)(MEM16(cur + 0x10) -
        shc_shift_right_signed((MEM16(cur + 0x10) - MEM16(prev + 0x10)) * MEM32(0x060e1298 + ch * 4), MEM16(cur + 2)));
    MEM16(arg0 + 0x26) = (s16)(MEM16(cur + 0x12) -
        shc_shift_right_signed((MEM16(cur + 0x12) - MEM16(prev + 0x12)) * MEM32(0x060e1298 + ch * 4), MEM16(cur + 2)));
    MEM16(arg0 + 0x28) = (s16)(MEM16(cur + 0x14) -
        shc_shift_right_signed((MEM16(cur + 0x14) - MEM16(prev + 0x14)) * MEM32(0x060e1298 + ch * 4), MEM16(cur + 2)));

    if (MEM32(0x060e1298 + ch * 4) == 0) return MEM16(cur + 0x18);
    return MEM16(cur);
}

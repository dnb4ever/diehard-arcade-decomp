/* display_ramp_to_zero [0x022100f8]: Seeds a signed display triple and ramps it to zero before notifying the parent. */
/* 0x022100f8 (recoil_axis_ramp_toward_zero) - per-frame update of one recoil/shake
 * channel on the display record base = ctrl->unk8, driven by speed = base->unk50 and
 * phase = ctrl->unk10 (a 3-state machine over the s16 triple at base[0x34/0x36/0x38],
 * a flag bit 2 at (ctrl->unk14->unk8)->unk3C):
 *   phase 0: raise flag bit 2; if speed>0 seed the triple to -0xFF and go phase 1,
 *            else seed to +0xFF and go phase 2.
 *   phase 1: base[0x34] += speed; once it reaches >=0 clear the triple, drop flag
 *            bit 2 and reset ctrl->unk0=0, else also step base[0x36]/base[0x38].
 *   phase 2: base[0x34] += speed; once it reaches <=0 clear/reset as above, else
 *            step base[0x36]/base[0x38].
 * Always ends by tail-calling func_022386fc(&base[0x34]) to commit the channel.
 * NOTE: fn.extent merges this entry with two sibling updaters at 0x2101ce and
 * 0x210298 (the braf tail-branch is not treated as a terminator); this TU models
 * only 0x2100f8's own body, which ends at that braf.
 * fidelity: HIGH - offsets 0x34/0x36/0x38 (s16), 0x3C flag, 0x50 speed, phase at
 *   0x10 (s8), sign tests (cmp/pz, cmp/pl), braf tail-call resolved to 0x022386fc. */
#include "dh_band21.h"

extern void display_accumulate_channel(u32 channel);   /* commit recoil channel (tail call) */

void display_ramp_to_zero(u32 ctrl) {
    u32 base  = MEM32(ctrl + 8);
    s32 speed = MEM32(base + 0x50);
    s32 phase = MEM8(ctrl + 0x10);
    s16 v;

    switch (phase) {
    case 0:
        MEM32(MEM32(MEM32(ctrl + 0x14) + 8) + 0x3c) |= 2;
        if (speed > 0) {
            MEM16(base + 0x38) = -0xff;
            MEM16(base + 0x36) = -0xff;
            MEM16(base + 0x34) = -0xff;
            MEM8(ctrl + 0x10) = 1;
        } else {
            MEM16(base + 0x38) = 0xff;
            MEM16(base + 0x36) = 0xff;
            MEM16(base + 0x34) = 0xff;
            MEM8(ctrl + 0x10) = 2;
        }
        break;
    case 1:
        v = (s16)(MEM16(base + 0x34) + speed);
        MEM16(base + 0x34) = v;
        if (v >= 0) {
            MEM16(base + 0x38) = 0;
            MEM16(base + 0x36) = 0;
            MEM16(base + 0x34) = 0;
            MEM32(MEM32(MEM32(ctrl + 0x14) + 8) + 0x3c) &= ~2;
            MEM32(ctrl + 0) = 0;
        } else {
            MEM16(base + 0x36) += speed;
            MEM16(base + 0x38) += speed;
        }
        break;
    case 2:
        v = (s16)(MEM16(base + 0x34) + speed);
        MEM16(base + 0x34) = v;
        if (v <= 0) {
            MEM16(base + 0x38) = 0;
            MEM16(base + 0x36) = 0;
            MEM16(base + 0x34) = 0;
            MEM32(MEM32(MEM32(ctrl + 0x14) + 8) + 0x3c) &= ~2;
            MEM32(ctrl + 0) = 0;
        } else {
            MEM16(base + 0x36) += speed;
            MEM16(base + 0x38) += speed;
        }
        break;
    }

    display_accumulate_channel(base + 0x34);
}

/* display_tick_intensity_ramp [0x0221039a]: Three task phases drive work+40/+44 from +50 and update display state. */
/* 0x0221039a (obj_intensity_ramp) - per-frame fade ramp for the display record
 * base = ctrl->unk8, a 3-state machine on phase = ctrl->unk10 that drives the
 * 16.16 fixed-point fields base[0x40]/base[0x44] from base[0x50] and pokes a 4-bit
 * intensity nibble into the live object *0x06098f08 (unk22). Flag bit 4 of
 * (ctrl->unk14->unk8)->unk3C marks the ramp active.
 *   phase 0: set flag bit 4; base[0x40] = sh2_hw_divide(15.0, base[0x50]<<16);
 *            base[0x44] = fxmul(base[0x50]<<16, base[0x40]); phase++ (fall through).
 *   phase 1: level = 15 - hi16(base[0x44]); obj->unk22 keeps its low byte and takes
 *            (level<<8)|(level<<12); base[0x44] = fxmul(base[0x50]<<16, base[0x40]);
 *            if base[0x44] < 1.0 advance phase; base[0x50]--.
 *   phase 2: clear flag bit 4; ctrl->unk0 = 0 (deactivate controller).
 * fidelity: HIGH - offsets 0x40/0x44/0x50 (16.16), 0x3C flag bit 4, phase at 0x10
 *   (s8), fxmul = func_022131c8 ((s64)a*b>>16), sh2_hw_divide resolved from bsrf.
 *   sh2_hw_divide (band leaf, not yet decompiled) modelled as returning s32. */
#include "dh_band21.h"

extern s32 fixed_mul16(s32 a, s32 b);   /* 16.16 fixed-point multiply */
extern s32 sh2_hw_divide(s32 a, s32 b);

void display_tick_intensity_ramp(u32 ctrl) {
    u32 base  = MEM32(ctrl + 8);
    u32 obj14 = MEM32(MEM32(ctrl + 0x14) + 8);
    s32 phase = MEM8(ctrl + 0x10);

    switch (phase) {
    case 0:
        MEM32(obj14 + 0x3c) |= 4;
        MEM32(base + 0x40) = sh2_hw_divide(0x000f0000, MEM32(base + 0x50) << 16);
        MEM32(base + 0x44) = fixed_mul16(MEM32(base + 0x50) << 16, MEM32(base + 0x40));
        MEM8(ctrl + 0x10) = (s8)(MEM8(ctrl + 0x10) + 1);
        /* fallthrough */
    case 1: {
        u32 obj = MEM32(DH_VDP2_REGISTER_SHADOW_ADDR);
        s32 level = 15 - (s16)(MEMU32(base + 0x44) >> 16);
        MEM16(obj + 0x22) = (s16)((MEM16(obj + 0x22) & 0xff) | (level << 8) | (level << 12));
        MEM32(base + 0x44) = fixed_mul16(MEM32(base + 0x50) << 16, MEM32(base + 0x40));
        if (MEM32(base + 0x44) < 0x00010000) {
            MEM8(ctrl + 0x10) = (s8)(MEM8(ctrl + 0x10) + 1);
        }
        MEM32(base + 0x50) -= 1;
        return;
    }
    case 2:
        MEM32(obj14 + 0x3c) &= ~4;
        MEM32(ctrl + 0) = 0;
        return;
    }
}

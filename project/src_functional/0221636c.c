/* vdp2_register_attribute_pair [0x0221636c]: Stores a twenty-four-byte paired-buffer attribute descriptor and applies its initial shadow value. */
#include "dh_band21.h"
/* 0x0221636c - sibling of func_0221632e: register a gouraud/color attribute command
 * into the per-command array at 0x06099514 (slot = (s8)((cmd-4)*0x18); no counter
 * cap) with record {cmd, 1, arg1, arg2, arg3, arg5}, then fall into the shared
 * attribute dispatcher (code at 0x022163b4, inlined as attr_dispatch) with cmd=arg0
 * and value=arg3. arg5 is the 5th (stack) argument.
 * fidelity: HIGH - 11-entry braf table decoded; byte-indexed slot, record layout
 *   (+0x0c=arg2 unlike the sibling), value scaling (val+0xda200000)>>1, and the
 *   cmd6/cmd7 b6-bitfield + secondary store modelled. */

static void attr_dispatch(s32 cmd, s32 value)
{
    u32 spr = MEM32(DH_VDP2_REGISTER_SHADOW_ADDR);
    s32 r5v = ((s32)(value + 0xda200000)) >> 1;
    s32 r14 = r5v << 1;
    u32 f   = MEMU16(spr + 0xb6);

    if ((u32)(cmd - 1) >= 11) {
        return;
    }
    switch (cmd) {
    case 1:  MEM32(spr + 0xa0) = r5v; break;
    case 2:  MEM32(spr + 0xa4) = r5v; break;
    case 3:  MEM32(spr + 0x9c) = r5v; break;
    case 4:
    case 5:  MEM32(spr + 0xbc) = r5v; break;
    case 6: {
        s32 adj = (r14 < 0) ? r14 + 0x3ffff : r14;
        s16 ins = (s16)(((u32)(adj >> 2)) >> 16);
        MEM16(spr + 0xb6) = (s16)((f & 0xfff8) | ((u16)ins));
        MEM32(MEM32(0x060b8b90) + 0x54) = r14 << 14;
        break;
    }
    case 7: {
        s32 adj = (r14 < 0) ? r14 + 0x3ffff : r14;
        s16 ins = (s16)(((u32)(adj >> 2)) >> 16);
        MEM16(spr + 0xb6) = (s16)((f & 0xf8ff) | (((u32)(u16)ins) << 8));
        MEM32(MEM32(0x060b8b94) + 0x54) = r14 << 14;
        break;
    }
    case 8:  MEM32(spr + 0xa8) = (0xfff80000u & MEMU32(spr + 0xa8)) | r5v | 0x80000000u; break;
    case 9:  MEM32(spr + 0xac) = (0xfff80000u & MEMU32(spr + 0xac)) | r5v | 0x80000000u; break;
    case 10: MEM32(spr + 0xd8) = (0xfff80000u & MEMU32(spr + 0xd8)) | r5v; break;
    case 11: MEM32(spr + 0xdc) = (0xfff80000u & MEMU32(spr + 0xdc)) | r5v; break;
    }
}

void vdp2_register_attribute_pair(s32 cmd, s32 arg1, s32 arg2, s32 arg3, s32 arg5)
{
    u32 rec = 0x06099514 + (u32)(s8)((cmd - 4) * 0x18);

    MEM32(rec + 0)  = cmd;
    MEM32(rec + 4)  = 1;
    MEM32(rec + 8)  = arg1;
    MEM32(rec + 12) = arg2;
    MEM32(rec + 16) = arg3;
    MEM32(rec + 20) = arg5;

    attr_dispatch(cmd, arg3);
}

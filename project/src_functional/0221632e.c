/* vdp2_queue_attribute [0x0221632e]: Appends a bounded 24-byte attribute record and applies its initial shadow value. */
#include "dh_band21.h"
/* 0x0221632e - register a gouraud/color attribute command (counter-bounded) then
 * apply it. Appends a 0x18-byte record to the array at 0x06098f0c, indexed by the
 * counter at 0x0609950c (capped at 64 - when full the call returns without doing
 * anything); record = {cmd, 1, arg1, -, arg2, arg3}. It then falls into the shared
 * attribute dispatcher (code at 0x022163b4, inlined as attr_dispatch) with the
 * command cmd=arg0 and value=arg2. Sibling entry func_0221636c shares that tail.
 * fidelity: HIGH - 11-entry braf table decoded (base braf_addr+4); value scaling
 *   (val+0xda200000)>>1, the spr=*0x06098f08 field offsets, and the cmd6/cmd7
 *   b6-bitfield + secondary store (<<14 to *0x060b8b90/94 +0x54) all modelled. */

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

void vdp2_queue_attribute(s32 cmd, s32 arg1, s32 arg2, s32 arg3)
{
    s32 cnt = MEM32(0x0609950c);
    u32 rec;

    if (cnt >= 64) {
        return;
    }
    MEM32(0x0609950c) = cnt + 1;
    rec = 0x06098f0c + (u32)(cnt * 0x18);
    MEM32(rec + 0)  = cmd;
    MEM32(rec + 4)  = 1;
    MEM32(rec + 8)  = arg1;
    MEM32(rec + 16) = arg2;
    MEM32(rec + 20) = arg3;

    attr_dispatch(cmd, arg2);
}

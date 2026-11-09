/* vdp2_apply_text_layer_commands [0x0222109e]: Interprets command/value pairs into the text background layer shadow fields. */
#include "dh.h"
/* 0x0222109e - VDP2 register-shadow field-packer driven by a command stream (cfg ptr in r4).
 * Sibling of func_02220828 / func_02220ade: identical shape (list of (command,value) s32 pairs
 * terminated by command==0; r5 = *g_06098f08 shadow block; 51-entry computed jump table with
 * target = 0x22111E + table[cmd-1]), but this variant sets bit3 of shadow[0x20] on entry
 * (shadow[0x20] = (shadow[0x20] & 0xFFF7) | 8) and packs a different field map (and has more
 * no-op command slots).  For each pair with (u32)(cmd-1) < 51: shadow[off] = (shadow[off] &
 * mask) | (value << shift), 16-bit; cmds 40/41 set bit31 of a 32-bit shadow word from the value
 * parity flag.  Commands 3,4,13..16,18..32 are no-ops.
 * fidelity: HIGH - every case hand-traced from the fn.py byte disassembly (jump table decoded
 * manually; m2c mis-disassembles it). Shifts counted from the chained shll/shll2/shll8 across
 * shared write-back tails; masks are the raw 16-bit AND constants. flag = (value&1)?0x80000000:0. */

extern void *g_vdp2_register_shadow_legacy;   /* VDP2 shadow-register block base */

void vdp2_apply_text_layer_commands(s32 *stream)
{
    void *s = g_vdp2_register_shadow_legacy;
    s32 cmd, value;

    MU16(s, 0x20) = (u16)((MU16(s, 0x20) & 0xfff7) | 8);

    while ((cmd = *stream++) != 0) {
        value = *stream++;
        if ((u32)(cmd - 1) >= 51)
            continue;
        switch (cmd) {
        case 1:  MU16(s, 0x20) = (u16)((MU16(s, 0x20) & 0xf7ff) | (value << 11)); break;
        case 2:  MU16(s, 0x2a) = (u16)((MU16(s, 0x2a) & 0xffdf) | (value << 5));  break;
        /* cases 3,4: no-op */
        case 5:  MU16(s, 0x2a) = (u16)((MU16(s, 0x2a) & 0xffef) | (value << 4));  break;
        case 6:  MU16(s, 0x36) = (u16)((MU16(s, 0x36) & 0x7fff) | (value << 15)); break;
        case 7:  MU16(s, 0x36) = (u16)((MU16(s, 0x36) & 0xbfff) | (value << 14)); break;
        case 8:  MU16(s, 0x36) = (u16)((MU16(s, 0x36) & 0xfdff) | (value << 9));  break;
        case 9:  MU16(s, 0x36) = (u16)((MU16(s, 0x36) & 0xfeff) | (value << 8));  break;
        case 10: MU16(s, 0x36) = (u16)((MU16(s, 0x36) & 0xff1f) | (value << 5));  break;
        case 11: MU16(s, 0x36) = (u16)((MU16(s, 0x36) & 0xffe0) | value);         break;
        case 12: MU16(s, 0x3a) = (u16)((MU16(s, 0x3a) & 0xff3f) | (value << 6));  break;
        /* cases 13..16: no-op */
        case 17: MU16(s, 0x22) = (u16)((MU16(s, 0x22) & 0xfff7) | (value << 3));  break;
        /* cases 18..32: no-op */
        case 33: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0x7fff) | (value << 15)); break;
        case 34: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0xfdff) | (value << 9));  break;
        case 35: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0xf7ff) | (value << 11)); break;
        case 36: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0xdfff) | (value << 13)); break;
        case 37: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0xfeff) | (value << 8));  break;
        case 38: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0xfbff) | (value << 10)); break;
        case 39: MU16(s, 0xd2) = (u16)((MU16(s, 0xd2) & 0xefff) | (value << 12)); break;
        case 40: M32(s, 0xd8) = (M32(s, 0xd8) & 0x7fffffff) | ((value & 1) ? (s32)0x80000000 : 0); break;
        case 41: M32(s, 0xdc) = (M32(s, 0xdc) & 0x7fffffff) | ((value & 1) ? (s32)0x80000000 : 0); break;
        case 42: MU16(s, 0xe4)  = (u16)((MU16(s, 0xe4)  & 0x8fff) | (value << 12)); break;
        case 43: MU16(s, 0x24)  = (u16)((MU16(s, 0x24)  & 0xfff7) | (value << 3));  break;
        case 44: MU16(s, 0xea)  = (u16)((MU16(s, 0xea)  & 0xff3f) | (value << 6));  break;
        case 45: MU16(s, 0xe8)  = (u16)((MU16(s, 0xe8)  & 0xfff7) | (value << 3));  break;
        case 46: MU16(s, 0xec)  = (u16)((MU16(s, 0xec)  & 0xfff7) | (value << 3));  break;
        case 47: MU16(s, 0x10a) = (u16)((MU16(s, 0x10a) & 0xe0ff) | (value << 8));  break;
        case 48: MU16(s, 0xee)  = (u16)((MU16(s, 0xee)  & 0xff3f) | (value << 6));  break;
        case 49: MU16(s, 0x110) = (u16)((MU16(s, 0x110) & 0xfff7) | (value << 3));  break;
        case 50: MU16(s, 0x112) = (u16)((MU16(s, 0x112) & 0xfff7) | (value << 3));  break;
        case 51: MU16(s, 0xe2)  = (u16)((MU16(s, 0xe2)  & 0xfff7) | (value << 3));  break;
        default: break;
        }
    }
}

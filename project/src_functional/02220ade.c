/* vdp2_apply_layer1_commands [0x02220ade]: Interprets command/value pairs into the second background layer shadow fields. */
#include "dh.h"
/* 0x02220ade - VDP2 register-shadow field-packer driven by a command stream (cfg ptr in r4).
 * Sibling of func_02220828 / func_0222109e: identical shape (list of (command,value) s32 pairs
 * terminated by command==0; r5 = *g_06098f08 shadow block; 51-entry computed jump table with
 * target = 0x220B6A + table[cmd-1]), but this variant sets bit1 of shadow[0x20] on entry
 * (shadow[0x20] = (shadow[0x20] & 0xFFFD) | 2) and packs a different field map.  For each pair
 * with (u32)(cmd-1) < 51: shadow[off] = (shadow[off] & mask) | (value << shift), 16-bit; cmds
 * 40/41 set bit31 of a 32-bit shadow word from the value parity flag.  Commands 13 and 24..32
 * are no-ops.
 * fidelity: HIGH - every case hand-traced from the fn.py byte disassembly (jump table decoded
 * manually; m2c mis-disassembles it). Shifts counted from the chained shll/shll2/shll8 across
 * shared write-back tails; masks are the raw 16-bit AND constants. flag = (value&1)?0x80000000:0. */

extern void *g_vdp2_register_shadow_legacy;   /* VDP2 shadow-register block base */

void vdp2_apply_layer1_commands(s32 *stream)
{
    void *s = g_vdp2_register_shadow_legacy;
    s32 cmd, value;

    MU16(s, 0x20) = (u16)((MU16(s, 0x20) & 0xfffd) | 2);

    while ((cmd = *stream++) != 0) {
        value = *stream++;
        if ((u32)(cmd - 1) >= 51)
            continue;
        switch (cmd) {
        case 1:  MU16(s, 0x20) = (u16)((MU16(s, 0x20) & 0xfdff) | (value << 9));  break;
        case 2:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xcfff) | (value << 12)); break;
        case 3:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xf3ff) | (value << 10)); break;
        case 4:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xfdff) | (value << 9));  break;
        case 5:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xfeff) | (value << 8));  break;
        case 6:  MU16(s, 0x32) = (u16)((MU16(s, 0x32) & 0x7fff) | (value << 15)); break;
        case 7:  MU16(s, 0x32) = (u16)((MU16(s, 0x32) & 0xbfff) | (value << 14)); break;
        case 8:  MU16(s, 0x32) = (u16)((MU16(s, 0x32) & 0xfdff) | (value << 9));  break;
        case 9:  MU16(s, 0x32) = (u16)((MU16(s, 0x32) & 0xfeff) | (value << 8));  break;
        case 10: MU16(s, 0x32) = (u16)((MU16(s, 0x32) & 0xff1f) | (value << 5));  break;
        case 11: MU16(s, 0x32) = (u16)((MU16(s, 0x32) & 0xffe0) | value);         break;
        case 12: MU16(s, 0x3a) = (u16)((MU16(s, 0x3a) & 0xfff3) | (value << 2));  break;
        /* case 13: no-op */
        case 14: MU16(s, 0x2c) = (u16)((MU16(s, 0x2c) & 0xdfff) | (value << 13)); break;
        case 15: MU16(s, 0x2c) = (u16)((MU16(s, 0x2c) & 0xefff) | (value << 12)); break;
        case 16: MU16(s, 0x2c) = (u16)((MU16(s, 0x2c) & 0xf8ff) | (value << 8));  break;
        case 17: MU16(s, 0x22) = (u16)((MU16(s, 0x22) & 0xfffd) | (value << 1));  break;
        case 18: MU16(s, 0x98) = (u16)((MU16(s, 0x98) & 0xfcff) | (value << 8));  break;
        case 19: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xcfff) | (value << 12)); break;
        case 20: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xf7ff) | (value << 11)); break;
        case 21: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfbff) | (value << 10)); break;
        case 22: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfdff) | (value << 9));  break;
        case 23: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfeff) | (value << 8));  break;
        /* cases 24..32: no-op */
        case 33: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0x7fff) | (value << 15)); break;
        case 34: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfdff) | (value << 9));  break;
        case 35: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xf7ff) | (value << 11)); break;
        case 36: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xdfff) | (value << 13)); break;
        case 37: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfeff) | (value << 8));  break;
        case 38: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfbff) | (value << 10)); break;
        case 39: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xefff) | (value << 12)); break;
        case 40: M32(s, 0xd8) = (M32(s, 0xd8) & 0x7fffffff) | ((value & 1) ? (s32)0x80000000 : 0); break;
        case 41: M32(s, 0xdc) = (M32(s, 0xdc) & 0x7fffffff) | ((value & 1) ? (s32)0x80000000 : 0); break;
        case 42: MU16(s, 0xe4)  = (u16)((MU16(s, 0xe4)  & 0xff8f) | (value << 4)); break;
        case 43: MU16(s, 0x24)  = (u16)((MU16(s, 0x24)  & 0xfffd) | (value << 1)); break;
        case 44: MU16(s, 0xea)  = (u16)((MU16(s, 0xea)  & 0xfff3) | (value << 2)); break;
        case 45: MU16(s, 0xe8)  = (u16)((MU16(s, 0xe8)  & 0xfffd) | (value << 1)); break;
        case 46: MU16(s, 0xec)  = (u16)((MU16(s, 0xec)  & 0xfffd) | (value << 1)); break;
        case 47: MU16(s, 0x108) = (u16)((MU16(s, 0x108) & 0xe0ff) | (value << 8)); break;
        case 48: MU16(s, 0xee)  = (u16)((MU16(s, 0xee)  & 0xfff3) | (value << 2)); break;
        case 49: MU16(s, 0x110) = (u16)((MU16(s, 0x110) & 0xfffd) | (value << 1)); break;
        case 50: MU16(s, 0x112) = (u16)((MU16(s, 0x112) & 0xfffd) | (value << 1)); break;
        case 51: MU16(s, 0xe2)  = (u16)((MU16(s, 0xe2)  & 0xfffd) | (value << 1)); break;
        default: break;
        }
    }
}

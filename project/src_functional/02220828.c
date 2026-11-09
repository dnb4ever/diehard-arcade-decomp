/* vdp2_apply_layer0_commands [0x02220828]: Interprets command/value pairs into the first background layer shadow fields. */
#include "dh.h"
/* 0x02220828 - VDP2 register-shadow field-packer driven by a command stream (cfg ptr in r4).
 * The stream is a list of (command, value) s32 pairs terminated by command==0.  r5 points at
 * the VDP2 shadow-register block (*g_06098f08).  On entry it sets bit0 of shadow[0x20]
 * (shadow[0x20] = (shadow[0x20] & 0xFFFE) | 1), then for each pair whose (u32)(cmd-1) < 51 it
 * packs the low bits of the value into a specific shadow register field via a 51-entry computed
 * jump table: field = (shadow[off] & mask) | (value << shift), stored 16-bit (two commands,
 * 40/41, instead set bit31 of a 32-bit shadow word from the value's parity flag).  The jump
 * table dispatch is `mova 0x220894 / mov.w @(r0,idx*2) / braf` with target = 0x220892 + entry;
 * commands 13 and 24..32 are no-ops (default table slot 0x022E -> loop tail).
 * This is func_02220828 (parent symbol: VDP2 register-config block loader).
 * fidelity: HIGH - every case hand-traced from the fn.py byte disassembly (jump table decoded
 * manually; m2c mis-disassembles it). Shifts counted from the chained shll/shll2/shll8 across
 * the shared write-back tails; masks are the raw 16-bit AND constants. Command 10
 * ANDs 0xFF1F and shifts <<5 (two shll2 plus shll; round-2 ROM correction). flag = (value & 1) ? 0x80000000 : 0 (rotr). */

extern void *g_vdp2_register_shadow_legacy;   /* VDP2 shadow-register block base */

void vdp2_apply_layer0_commands(s32 *stream)
{
    void *s = g_vdp2_register_shadow_legacy;
    s32 cmd, value;

    MU16(s, 0x20) = (u16)((MU16(s, 0x20) & 0xfffe) | 1);

    while ((cmd = *stream++) != 0) {
        value = *stream++;
        if ((u32)(cmd - 1) >= 51)
            continue;
        switch (cmd) {
        case 1:  MU16(s, 0x20) = (u16)((MU16(s, 0x20) & 0xfeff) | (value << 8));  break;
        case 2:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xff8f) | (value << 4));  break;
        case 3:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xfff3) | (value << 2));  break;
        case 4:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xfffd) | (value << 1));  break;
        case 5:  MU16(s, 0x28) = (u16)((MU16(s, 0x28) & 0xfffe) | value);         break;
        case 6:  MU16(s, 0x30) = (u16)((MU16(s, 0x30) & 0x7fff) | (value << 15)); break;
        case 7:  MU16(s, 0x30) = (u16)((MU16(s, 0x30) & 0xbfff) | (value << 14)); break;
        case 8:  MU16(s, 0x30) = (u16)((MU16(s, 0x30) & 0xfdff) | (value << 9));  break;
        case 9:  MU16(s, 0x30) = (u16)((MU16(s, 0x30) & 0xfeff) | (value << 8));  break;
        case 10: MU16(s, 0x30) = (u16)((MU16(s, 0x30) & 0xff1f) | (value << 5)); break;
        case 11: MU16(s, 0x30) = (u16)((MU16(s, 0x30) & 0xffe0) | value);         break;
        case 12: MU16(s, 0x3a) = (u16)((MU16(s, 0x3a) & 0xfffc) | value);         break;
        /* case 13: no-op */
        case 14: MU16(s, 0x2c) = (u16)((MU16(s, 0x2c) & 0xffdf) | (value << 5));  break;
        case 15: MU16(s, 0x2c) = (u16)((MU16(s, 0x2c) & 0xffef) | (value << 4));  break;
        case 16: MU16(s, 0x2c) = (u16)((MU16(s, 0x2c) & 0xfff8) | value);         break;
        case 17: MU16(s, 0x22) = (u16)((MU16(s, 0x22) & 0xfffe) | value);         break;
        case 18: MU16(s, 0x98) = (u16)((MU16(s, 0x98) & 0xfffc) | value);         break;
        case 19: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xffcf) | (value << 4));  break;
        case 20: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfff7) | (value << 3));  break;
        case 21: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfffb) | (value << 2));  break;
        case 22: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfffd) | (value << 1));  break;
        case 23: MU16(s, 0x9a) = (u16)((MU16(s, 0x9a) & 0xfffe) | value);         break;
        /* cases 24..32: no-op */
        case 33: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xff7f) | (value << 7));  break;
        case 34: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfffd) | (value << 1));  break;
        case 35: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfff7) | (value << 3));  break;
        case 36: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xffdf) | (value << 5));  break;
        case 37: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfffe) | value);         break;
        case 38: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xfffb) | (value << 2));  break;
        case 39: MU16(s, 0xd0) = (u16)((MU16(s, 0xd0) & 0xffef) | (value << 4));  break;
        case 40: M32(s, 0xd8) = (M32(s, 0xd8) & 0x7fffffff) | ((value & 1) ? (s32)0x80000000 : 0); break;
        case 41: M32(s, 0xdc) = (M32(s, 0xdc) & 0x7fffffff) | ((value & 1) ? (s32)0x80000000 : 0); break;
        case 42: MU16(s, 0xe4)  = (u16)((MU16(s, 0xe4)  & 0xfff8) | value); break;
        case 43: MU16(s, 0x24)  = (u16)((MU16(s, 0x24)  & 0xfffe) | value); break;
        case 44: MU16(s, 0xea)  = (u16)((MU16(s, 0xea)  & 0xfffc) | value); break;
        case 45: MU16(s, 0xe8)  = (u16)((MU16(s, 0xe8)  & 0xfffe) | value); break;
        case 46: MU16(s, 0xec)  = (u16)((MU16(s, 0xec)  & 0xfffe) | value); break;
        case 47: MU16(s, 0x108) = (u16)((MU16(s, 0x108) & 0xffe0) | value); break;
        case 48: MU16(s, 0xee)  = (u16)((MU16(s, 0xee)  & 0xfffc) | value); break;
        case 49: MU16(s, 0x110) = (u16)((MU16(s, 0x110) & 0xfffe) | value); break;
        case 50: MU16(s, 0x112) = (u16)((MU16(s, 0x112) & 0xfffe) | value); break;
        case 51: MU16(s, 0xe2)  = (u16)((MU16(s, 0xe2)  & 0xfffe) | value); break;
        default: break;
        }
    }
}

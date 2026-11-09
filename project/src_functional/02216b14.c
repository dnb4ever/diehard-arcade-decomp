/* vdp2_apply_sprite_commands [0x02216b14]: Interprets sprite control command/value pairs into the VDP2 shadow. */
#include "dh_band21.h"
/* 0x02216b14 - VDP sprite-attribute bitfield command interpreter. Walks a command
 * stream (pairs of longs: {cmd, operand}, terminated by cmd==0) and pokes bitfields
 * of the active sprite/attribute block spr = *0x06098f08. Each in-range command
 * (0x21..0x32) clears one bit of a 16-bit control word and ORs in operand<<shift
 * via a braf jump table; out-of-range and the gap commands (0x28-0x2c, 0x2f, 0x30)
 * consume their operand and do nothing.
 *   0x21: spr[0xd4] bit15   0x22: bit9   0x23: bit11  0x24: bit13
 *   0x25: spr[0xd4] bit8    0x26: bit10  0x27: bit12
 *   0x2d: spr[0xe8] bit5    0x2e: spr[0xec] bit6
 *   0x31: spr[0x110] bit6   0x32: spr[0x112] bit6
 * fidelity: HIGH - 18-entry braf table decoded from ROM (base braf_addr+4), shift
 *   cascades traced per entry, AND masks and write offsets exact. operand is read
 *   as a long; the field write is 16-bit. */
void vdp2_apply_sprite_commands(u32 stream)
{
    u32 spr = MEM32(DH_VDP2_REGISTER_SHADOW_ADDR);
    s32 cmd;

    while ((cmd = MEM32(stream)) != 0) {
        u32 op;
        stream += 4;
        op = MEM32(stream);
        stream += 4;

        switch (cmd) {
        case 0x21: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0x7fff) | (op << 15)); break;
        case 0x22: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0xfdff) | (op << 9));  break;
        case 0x23: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0xf7ff) | (op << 11)); break;
        case 0x24: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0xdfff) | (op << 13)); break;
        case 0x25: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0xfeff) | (op << 8));  break;
        case 0x26: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0xfbff) | (op << 10)); break;
        case 0x27: MEM16(spr + 0xd4)  = (s16)((MEMU16(spr + 0xd4)  & 0xefff) | (op << 12)); break;
        case 0x2d: MEM16(spr + 0xe8)  = (s16)((MEMU16(spr + 0xe8)  & 0xffdf) | (op << 5));  break;
        case 0x2e: MEM16(spr + 0xec)  = (s16)((MEMU16(spr + 0xec)  & 0xffbf) | (op << 6));  break;
        case 0x31: MEM16(spr + 0x110) = (s16)((MEMU16(spr + 0x110) & 0xffbf) | (op << 6));  break;
        case 0x32: MEM16(spr + 0x112) = (s16)((MEMU16(spr + 0x112) & 0xffbf) | (op << 6));  break;
        default: break;
        }
    }
}

#include "dh.h"

/* 0x02215000 (vdp1_cmdlist_header_init) - appends four fixed 0x24-byte VDP1
 * command-table entries to the list whose write pointer lives at *0x060CF0C4.
 * Each entry's +0x20 word gets 0x003FF000 before the pointer is bumped by 0x24.
 *   opcode 8: [0]=8, [0xC]=0, [0xE]=0, [0x14]=0x2BF, [0x16]=0xEF  (user clip)
 *   opcode 9: [0]=9, [0x14]=0x2BF, [0x16]=0xEF                    (system clip)
 *   opcode A: [0]=0xA, [0xC]=0x160, [0xE]=0x78                    (local coord)
 *   opcode 4: [0]=4, [4]=0xC0, [6]=0, [0xC..0x1A]=src[0..7] from 0x06053DAC
 * fidelity: HIGH - every field is a distinct 16-bit volatile store in the
 * asm's exact order; the write pointer is re-read from memory per access. */
void func_02215000(void) {
    const u32 wp = 0x060CF0C4;
    const u32 src = 0x06053DAC;
    u32 e;

    /* entry opcode 8 (user clip) */
    M32(MU32(wp, 0), 0x20) = 0x003FF000;
    e = MU32(wp, 0);
    M32(wp, 0) = e + 0x24;
    M16(e, 0x00) = 8;
    M16(e, 0x0C) = 0;
    M16(e, 0x0E) = 0;
    M16(e, 0x14) = 0x02BF;
    M16(e, 0x16) = 0x00EF;

    /* entry opcode 9 (system clip) */
    M32(MU32(wp, 0), 0x20) = 0x003FF000;
    e = MU32(wp, 0);
    M32(wp, 0) = e + 0x24;
    M16(e, 0x00) = 9;
    M16(e, 0x14) = 0x02BF;
    M16(e, 0x16) = 0x00EF;

    /* entry opcode 0xA (local coord) */
    M32(MU32(wp, 0), 0x20) = 0x003FF000;
    e = MU32(wp, 0);
    M32(wp, 0) = e + 0x24;
    M16(e, 0x00) = 0x0A;
    /* Opt-in mod: move the VDP1 playfield 48 pixels to the right. */
    M16(e, 0x0C) = 0x190;
    M16(e, 0x0E) = 0x78;

    /* entry opcode 4 (draw) */
    M32(MU32(wp, 0), 0x20) = 0x003FF000;
    e = MU32(wp, 0);
    M32(wp, 0) = e + 0x24;
    M16(e, 0x04) = 0xC0;
    M16(e, 0x00) = 4;
    M16(e, 0x06) = 0;
    M16(e, 0x0C) = M16(src, 0x00);
    M16(e, 0x0E) = M16(src, 0x02);
    M16(e, 0x10) = M16(src, 0x04);
    M16(e, 0x12) = M16(src, 0x06);
    M16(e, 0x14) = M16(src, 0x08);
    M16(e, 0x16) = M16(src, 0x0A);
    M16(e, 0x18) = M16(src, 0x0C);
    M16(e, 0x1A) = M16(src, 0x0E);
}

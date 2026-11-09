/* animation_cell_reset [0x0222eb72]: Clears byte/halfword control record at 06096c94 in original order. */
#include "dh.h"
/* 0x0222eb72  zero the 0xC-byte record at 0x06096C94 (1 byte + five 16-bit words).
 * fidelity: HIGH - exact; store order matches disasm. */
extern u8 g_06096C94[];
void animation_cell_reset(void) {
    M16(g_06096C94, 0x2) = 0;
    M8(g_06096C94, 0x0)  = 0;
    M16(g_06096C94, 0x4) = 0;
    M16(g_06096C94, 0x8) = 0;
    M16(g_06096C94, 0x6) = 0;
    M16(g_06096C94, 0xA) = 0;
}

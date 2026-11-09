/* hud_encode_config_nibble [0x02203e5e]: Copies a two-byte status item selected by a machine configuration nibble. */
/* func_02203e5e (cart 0x203e5e) - leaf.
 * Copy a 2-byte table entry (table @0x060506F0) to 0x0609D7B0+arg0.
 * Index is a nibble of the byte @0x060987E9: low nibble when arg1!=0,
 * high nibble when arg1==0.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void hud_encode_config_nibble(s32 arg0, s32 arg1) {
    u8 *dst = (u8 *)(0x0609D7B0 + arg0);
    u8 v = *(u8 *)0x060987E9;
    s32 nib = (arg1 != 0) ? (v & 0xF) : (v >> 4);
    const s8 *tbl = (const s8 *)(0x060506F0 + nib * 4);

    dst[0] = (u8)tbl[0];
    dst[1] = (u8)tbl[1];
}

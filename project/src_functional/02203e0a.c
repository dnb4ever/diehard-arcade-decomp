/* hud_encode_status_item [0x02203e0a]: Encodes a five-byte status item from a table selector and two display values. */
/* func_02203e0a (cart 0x203e0a) - leaf.
 * Build a 5-byte record at 0x0609D7B0+arg0 from a 2-entry table lookup
 * (table @0x060506F0, index = arg1[0]) plus two computed bytes derived
 * from arg1[1]/arg1[2].
 * Confidence: HIGH. No calls. Faithful to disasm (uses a 3-byte scratch).
 */
#include "decomp.h"

void hud_encode_status_item(s32 arg0, u8 *arg1) {
    u8 sp[3];
    u8 *dst = (u8 *)(0x0609D7B0 + arg0);
    const s8 *tbl = (const s8 *)(0x060506F0 + (u32)arg1[0] * 4);

    dst[0] = (u8)tbl[0];
    dst[1] = (u8)tbl[1];
    if (arg1[1] != 0) {
        sp[0] = (u8)(arg1[1] + 15);
        sp[1] = (u8)(arg1[2] + 22);
    } else {
        sp[0] = 32;
        sp[1] = 32;
    }
    sp[2] = 0;
    dst[2] = sp[0];
    dst[3] = sp[1];
    dst[4] = sp[2];
}

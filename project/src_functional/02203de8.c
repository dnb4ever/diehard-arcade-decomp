/* hud_copy_status_template [0x02203de8]: Copies the selected twenty-three-byte HUD status template into the text buffer. */
/* func_02203de8 (cart 0x203de8) - leaf.
 * Copy 23 bytes from a pointer taken from table[0x060902CC + arg0*4]
 * into the fixed buffer at 0x0609D7B0.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void hud_copy_status_template(s32 arg0) {
    const u8 *src = *(const u8 **)(0x060902CC + arg0 * 4);
    u8 *dst = (u8 *)0x0609D7B0;
    s32 i;
    for (i = 0; i < 23; i++)
        dst[i] = src[i];
}

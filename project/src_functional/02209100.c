/* background_emit_grayscale_ramp [0x02209100]: Writes 240 RGB555 levels through mode-9 display transfer cursor. */
/* func_02209100 (cart 0x209100) - callC.
 * Call func_0221632e(9, cursor) (cursor @0x06099510), then write a 240-entry
 * grayscale RGB555 ramp into CRAM via that cursor. Each level v=(i&0x3F)>>1 is
 * replicated into R/G/B ((v<<10)|(v<<5)|v)&0x7FFF; two entries are packed per
 * dword.
 * Call target: bsrf -> cart 0x21632E -> RAM 0x0601A32E.
 * Confidence: HIGH. Loop faithful to disasm.
 */
#include "decomp.h"

extern void *vdp2_queue_attribute(s32, void *);

void background_emit_grayscale_ramp(void) {
    s32 i;

    vdp2_queue_attribute(9, *(void **)DH_DISPLAY_TRANSFER_CURSOR_ADDR);

    for (i = 0; i < 0xF0; i += 2) {
        s32 v0 = (i & 0x3F) >> 1;
        s32 v1 = ((i + 1) & 0x3F) >> 1;
        u32 c0 = ((v0 << 10) | (v0 << 5) | v0) & 0x7FFF;
        u32 c1 = ((v1 << 10) | (v1 << 5) | v1) & 0x7FFF;
        u32 *cur = *(u32 **)DH_DISPLAY_TRANSFER_CURSOR_ADDR;
        *cur = c0 << 16;
        *(char **)DH_DISPLAY_TRANSFER_CURSOR_ADDR += 4;
        *cur |= c1;
    }
}

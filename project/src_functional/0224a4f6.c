/* shc_signfill_rotl3 [0x0224a4f6]: Existing map corroborated by source: ; 0604e4f6..e502 ROTL three times then OR fffffffc */
/* 0x0224a4f6: SHC arithmetic-shift sled, rotate left 3 then sign-fill.
 * ROM uses ROTL, so the rotation must use an unsigned word (logical right).
 * The dispatcher supplies r0 and a saved r2 stack slot; result leaves in r0.
 */
#include "dh.h"

s32 shc_signfill_rotl3(s32 value) {
    u32 rotated = (u32)value;
    rotated = (rotated << 3) | (rotated >> 29);
    return (s32)(rotated | (u32)-4);
}

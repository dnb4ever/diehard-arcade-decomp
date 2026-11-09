/* shc_signfill_rotl4 [0x0224a4e6]: Existing map corroborated by source: ; 0604e4e6..e4f4 ROTL four times then OR fffffff8 */
/* 0x0224a4e6: SHC arithmetic-shift sled, rotate left 4 then sign-fill.
 * ROM uses ROTL, so the rotation must use an unsigned word (logical right).
 * The dispatcher supplies r0 and a saved r2 stack slot; result leaves in r0.
 */
#include "dh.h"

s32 shc_signfill_rotl4(s32 value) {
    u32 rotated = (u32)value;
    rotated = (rotated << 4) | (rotated >> 28);
    return (s32)(rotated | (u32)-8);
}

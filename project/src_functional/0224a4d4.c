/* shc_signfill_rotl5 [0x0224a4d4]: Existing map corroborated by source: ; 0604e4d4..e4e4 ROTL five times then OR fffffff0 */
/* 0x0224a4d4: SHC arithmetic-shift sled, rotate left 5 then sign-fill.
 * ROM uses ROTL, so the rotation must use an unsigned word (logical right).
 * The dispatcher supplies r0 and a saved r2 stack slot; result leaves in r0.
 */
#include "dh.h"

s32 shc_signfill_rotl5(s32 value) {
    u32 rotated = (u32)value;
    rotated = (rotated << 5) | (rotated >> 27);
    return (s32)(rotated | (u32)-16);
}

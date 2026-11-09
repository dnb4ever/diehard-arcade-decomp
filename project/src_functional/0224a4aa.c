/* shc_signfill_rotl7 [0x0224a4aa]: Existing map corroborated by source: ; 0604e4aa..e4be ROTL seven times then OR ffffffc0 */
/* 0x0224a4aa: SHC arithmetic-shift sled, rotate left 7 then sign-fill.
 * ROM uses ROTL, so the rotation must use an unsigned word (logical right).
 * The dispatcher supplies r0 and a saved r2 stack slot; result leaves in r0.
 */
#include "dh.h"

s32 shc_signfill_rotl7(s32 value) {
    u32 rotated = (u32)value;
    rotated = (rotated << 7) | (rotated >> 25);
    return (s32)(rotated | (u32)-64);
}

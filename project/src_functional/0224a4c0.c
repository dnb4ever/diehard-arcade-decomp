/* shc_rotate_left6_signfill [0x0224a4c0]: Six left rotations followed by OR with -32 in runtime shift sled. */
#include "dh.h"

s32 shc_rotate_left6_signfill(u32 r0) {
    u32 temp;
    u32 temp_2;
    u32 temp_3;
    u32 temp_4;
    u32 temp_5;

    temp = (r0 << 1) | (r0 >> 31);
    temp_2 = (temp << 1) | (temp >> 31);
    temp_3 = (temp_2 << 1) | (temp_2 >> 31);
    temp_4 = (temp_3 << 1) | (temp_3 >> 31);
    temp_5 = (temp_4 << 1) | (temp_4 >> 31);
    temp = (temp_5 << 1) | (temp_5 >> 31);
    return (s32)(temp | -32);
}
/* fidelity: high - implements 6x rotl followed by OR with -32 */

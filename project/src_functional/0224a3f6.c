/* shc_mask_rotate_right1 [0x0224a3f6]: Runtime leaf masks then rotates input right one. */
#include "dh.h"

u32 shc_mask_rotate_right1(u32 r0) {
    u32 temp_r0;

    temp_r0 = r0 & 1;
    return (temp_r0 >> 1) | (temp_r0 << 0x1F);
} /* fidelity: high - correctly models r0 as input parameter and implements and/rotr sequence */

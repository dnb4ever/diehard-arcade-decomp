/* shc_mask_rotate_right3 [0x0224a3e0]: Runtime leaf applies three right rotations. */
#include "dh.h"

u32 shc_mask_rotate_right3(u32 arg0) {
    u32 temp = arg0 & 7;
    temp = (temp >> 3) | (temp << 29);
    return temp;
}

/* fidelity: high - direct translation of 3 rotr operations as 32-bit rotate-right by 3 */

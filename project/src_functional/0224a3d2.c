/* shc_mask_rotate_right4 [0x0224a3d2]: Runtime leaf masks then rotates input right four. */
#include "dh.h"

u32 shc_mask_rotate_right4(u32 r0) {
    r0 &= 15;
    r0 = (r0 >> 1) | (r0 << 31);
    r0 = (r0 >> 1) | (r0 << 31);
    r0 = (r0 >> 1) | (r0 << 31);
    r0 = (r0 >> 1) | (r0 << 31);
    return r0;
} /* fidelity: high - direct translation of and + 4x rotr on input r0 */

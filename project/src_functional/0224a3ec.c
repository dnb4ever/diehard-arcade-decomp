/* shc_mask_rotate_right2 [0x0224a3ec]: Runtime leaf applies two right rotations; misleading legacy comment retained as evidence history. */
#include "dh.h"

u32 shc_mask_rotate_right2(u32 r0) {
    r0 &= 3;
    r0 = (r0 >> 1) | (r0 << 31);
    r0 = (r0 >> 1) | (r0 << 31);
    return r0;
} /* fidelity: high - correctly models r0 as input parameter and implements two rotr instructions as rotate-right-by-1 */

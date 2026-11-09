/* 0x02213dec  avg_field: obj[+32] = (r1 + obj[+20] + obj[+24] + obj[+28]) >> 2  (logical).
 * Reached via the 0x2213d90 jump-table; on entry r8=obj, r1=value (dispatcher sets r1=obj[+16]).
 * fidelity: faithful - three adds, shlr2 (logical >>2), store in rts delay. Leaf. */
#include "dh_band21.h"

void avg_field(s32 *obj, s32 r1) {
    r1 += obj[5];   /* +20 */
    r1 += obj[6];   /* +24 */
    r1 += obj[7];   /* +28 */
    obj[8] = (s32)((u32)r1 >> 2);   /* +32 */
}

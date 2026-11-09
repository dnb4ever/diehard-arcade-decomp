/* 0x02213dd0  clamp_min_field: obj[+32] = min(r1, obj[+20], obj[+24], obj[+28]).
 * Reached via the 0x2213d90 jump-table; on entry r8=obj, r1=value (dispatcher sets r1=obj[+16]).
 * Modeled with the register ABI explicit: obj (r8) and r1 as parameters.
 * fidelity: faithful - three cmp/ge r1,rX + bt/s (skip r1=rX) => r1=min, store in rts delay. Leaf. */
#include "dh_band21.h"

void clamp_min_field(s32 *obj, s32 r1) {
    if (!(obj[5] >= r1)) r1 = obj[5];   /* +20 */
    if (!(obj[6] >= r1)) r1 = obj[6];   /* +24 */
    if (!(obj[7] >= r1)) r1 = obj[7];   /* +28 */
    obj[8] = r1;                        /* +32 */
}

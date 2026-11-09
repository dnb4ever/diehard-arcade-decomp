/* 0x02213354  aabb_overlap_3: 3-axis interval overlap of box [a,b] vs box [c,d].
 * For i in 0..2: require b[i] >= c[i] AND d[i] >= a[i]; else return 0. All pass -> 1. (signed cmp/ge)
 * fidelity: faithful - three unrolled axis blocks, each two cmp/ge + bf/s to the fail exit. Leaf. */
#include "dh_band21.h"

s32 aabb_overlap_3(s32 *a, s32 *b, s32 *c, s32 *d) {
    int i;
    for (i = 0; i < 3; i++) {
        if (!(b[i] >= c[i])) return 0;
        if (!(d[i] >= a[i])) return 0;
    }
    return 1;
}

/* 0x0221330c  bounds_overlap_margin: 3-axis range test of point/box vs center+-margin.
 * For i in 0..2: require b[i] >= (c[i]-margin) AND (c[i]+margin) >= a[i]; else return 0.
 * All three axes pass -> return 1. (signed cmp/ge)
 * fidelity: faithful - three unrolled axis blocks, each two cmp/ge + bf/s to the fail exit. Leaf. */
#include "dh_band21.h"

s32 bounds_overlap_margin(s32 *a, s32 *b, s32 *c, s32 margin) {
    int i;
    for (i = 0; i < 3; i++) {
        s32 lo = c[i] - margin;
        s32 hi = c[i] + margin;
        if (!(b[i] >= lo)) return 0;
        if (!(hi >= a[i])) return 0;
    }
    return 1;
}

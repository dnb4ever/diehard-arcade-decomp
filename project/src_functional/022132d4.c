/* 0x022132d4  vec3_minmax: per-component max/min of two 3-vectors.
 * For i in 0..2: if (b[i] > a[i]) { out_max[i]=b[i]; out_min[i]=a[i]; }
 *               else            { out_max[i]=a[i]; out_min[i]=b[i]; }   (signed cmp/gt)
 * fidelity: faithful - three unrolled cmp/gt + branch pairs, pointers post-incremented. Leaf. */
#include "dh_band21.h"

void vec3_minmax(s32 *a, s32 *b, s32 *out_max, s32 *out_min) {
    int i;
    for (i = 0; i < 3; i++) {
        if (b[i] > a[i]) {
            out_max[i] = b[i];
            out_min[i] = a[i];
        } else {
            out_max[i] = a[i];
            out_min[i] = b[i];
        }
    }
}

/* quad_select_depth [0x02213d90]: Six-entry selector computes quad depth at +20; r9/r8 ABI documented in source. */
/* 0x02213d90: select quad depth and store it at +0x20.
 * ROM 06017d9c is the six-entry jump table; 06017db4 is executable code.
 * Inputs are selector r9 and quad r8; the result leaves in r1.
 */
#include "dh.h"

s32 quad_select_depth(s32 selector, void *quad) {
    s32 depth = M32(quad, 0x10);
    s32 i;

    if (selector == 0) {
        for (i = 0x14; i <= 0x1c; i += 4)
            if (M32(quad, i) > depth) depth = M32(quad, i);
    } else if (selector == 1) {
        for (i = 0x14; i <= 0x1c; i += 4)
            if (M32(quad, i) < depth) depth = M32(quad, i);
    } else if (selector == 3) {
        depth = M32(quad, -0x10);
    } else {
        depth = (s32)(((u32)depth + MU32(quad, 0x14) +
                       MU32(quad, 0x18) + MU32(quad, 0x1c)) >> 2);
    }
    M32(quad, 0x20) = depth;
    return depth;
}

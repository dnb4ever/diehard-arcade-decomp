/* Optional convergence demo: halve the sine component of model rotations.
 * Based on the frozen project/src_functional/02213190.c. The only behavior
 * change is removal of the final << 1. This file is used only by --variant mod.
 */
#include "dh_band21.h"

s32 sin_lookup(s32 angle) {
    u16 idx = (u16)(angle + 8) >> 4;
    return (s32)MEM16(TRIG_TABLE + idx * 2);
}

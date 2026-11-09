/* angle_difference_nonnegative_halfturn [0x0223e5bc]: Returns whether bit 0x8000 of a-b is clear. */
#include "dh.h"
/* 0x0223e5bc (leafC) - return ((a - b) & 0x8000) == 0.
 * fidelity: HIGH - exact (sub, and #0x8000, tst, movt). */
s32 angle_difference_nonnegative_halfturn(s32 a, s32 b) {
    return ((a - b) & 0x8000) == 0;
}

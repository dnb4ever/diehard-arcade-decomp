/* vec2_magnitude_scaled [0x0221047c]: Absolute components shifted by eight, squared and summed before integer square root. */
/* 0x0221047c (vec2_magnitude) - 2D vector magnitude: take |a| and |b|, scale each
 * down by 8 bits, and tail-call the integer-sqrt helper func_02213164 on the sum of
 * their squares, i.e. sqrt((|a|>>8)^2 + (|b|>>8)^2).
 * NOTE: fn.extent merges this entry with two sibling helpers at 0x2104b4 (the 3D
 * form) and 0x2104f6 (the braf tail-branch is not treated as a terminator); this TU
 * models only 0x21047c's own body, which ends at that braf.
 * fidelity: HIGH - abs via cmp/pz+neg, >>8 is shlr8 (logical), mul.l low word
 *   (macl) squares, braf tail-call resolved to func_02213164. */
#include "dh_band21.h"

extern s32 fixed_sqrt16(s32 sumsq);   /* integer square root */

s32 vec2_magnitude_scaled(s32 a, s32 b) {
    u32 ua = (u32)(a < 0 ? -a : a) >> 8;
    u32 ub = (u32)(b < 0 ? -b : b) >> 8;
    return fixed_sqrt16((s32)(ua * ua + ub * ub));
}

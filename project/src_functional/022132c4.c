/* vector_dot_fixed16 [0x022132c4]: Returns the signed three-component dot product shifted down sixteen bits. */
#include "dh.h"
/* 0x022132c4 (vec3_dot_fixed) - 16.16 fixed-point 3-vector dot product. Accumulates
 * a[0]*b[0] + a[1]*b[1] + a[2]*b[2] as a signed 64-bit MAC (clrmac + 3x mac.l) and returns
 * the middle 32 bits (acc >> 16), i.e. the 16.16 result (sts mach/macl + xtrct r1,r0).
 * fidelity: HIGH - mac.l modeled as an explicit s64 multiply-accumulate; m2c cannot lower
 * mac.l (M2C_ERROR), so this is a hand transcription of the stereotyped MAC sequence. */
s32 vector_dot_fixed16(s32 *a, s32 *b) {
    s64 acc = (s64)a[0] * b[0] + (s64)a[1] * b[1] + (s64)a[2] * b[2];
    return (s32)(acc >> 16);
}

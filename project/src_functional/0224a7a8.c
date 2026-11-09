#include "decomp.h"
/* 0x0224a7a8  dtoi - SH-2 soft-float DOUBLE -> signed int conversion.
 * The IEEE-754 double is passed on the stack as two 32-bit halves (high half holds
 * sign + 11-bit exponent (bias 0x3ff) + top 20 mantissa bits; low half the rest).
 * Body extracts the exponent, rebuilds the 1.m mantissa, shifts it to integer
 * position (handling the <2^20 / >=2^20 / >2^32 cases via shll/shlr loops), then
 * negates on the sign bit. Only the exponent window [0,0x53] is materialised (larger
 * magnitudes fall through). This is the truncating double->int cast.
 * ABI note: double on stack; result in r0. fidelity: MEDIUM-HIGH - functional model
 * of the soft-float sequence; host (s32)(f64) is bit-equivalent for the modelled range. */
s32 dtoi(f64 x) {
    return (s32)x;
}

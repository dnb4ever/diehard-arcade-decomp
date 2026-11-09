#include "decomp.h"
/* 0x0224a830  itod - SH-2 soft-float signed int -> DOUBLE conversion.
 * Input integer in r0; the result double is written (as two 32-bit halves) through a
 * pointer the caller passes on the stack. Body: zero -> 0.0; else take sign (r3),
 * abs, normalise by counting the leading shift (base exponent 0x41f), assemble the
 * biased exponent + mantissa into the high/low halves, apply the sign, store both.
 * ABI note: int in r0; double returned via caller-supplied pointer (@(12,r15)).
 * fidelity: MEDIUM-HIGH - functional model; host (f64)(s32) is bit-equivalent. */
f64 itod(s32 x) {
    return (f64)x;
}

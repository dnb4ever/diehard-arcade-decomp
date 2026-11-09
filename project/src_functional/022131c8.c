/* fixed_mul16 [0x022131c8]: Signed 64-bit product shifted down by 16 bits. */
#include "dh.h"
/* 0x022131c8 (fixmul_16_16) - signed 16.16 fixed-point multiply. Computes the full
 * 64-bit signed product (dmuls.l) and returns bits 16..47, i.e. (a*b) >> 16.
 * fidelity: HIGH - dmuls.l + xtrct(mach,macl) == (s64 product) >> 16. Leaf. */
s32 fixed_mul16(s32 a, s32 b) {
    return (s32)(((s64)a * (s64)b) >> 16);
}

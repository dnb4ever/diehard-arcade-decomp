/* 0x02213ba0  mtx_rotate_x: rotate the active 3x4 matrix (M = *g_06098878, 3 rows of
 * 4 longs) about its column-0 axis by `angle`, mixing columns 1 and 2 of every row.
 * angle == 0 is a no-op (shared rts @0x213ca0). Uses sincos_lookup(-angle) (0x213f90).
 * per row:  m1=M[1], m2=M[2];
 *   M[1] = (c*m1 + s*m2) >> 16;  M[2] = (-s*m1 + c*m2) >> 16    (both from originals)
 * fidelity: HIGH - sincos via 0x213f90 on the negated angle; stack 2x2 [-s,c,c,s];
 *   clrmac/mac.l 2-term/xtrct(>>16) per column, dt r1 (3 rows) loop, 16-byte stride. */
#include "dh_band21.h"

void sincos_lookup(s32 angle, s32 *out_sin, s32 *out_cos);

void mtx_rotate_x(s32 angle) {
    s32 *M;
    s32 s, c;
    int r;

    if (angle == 0) return;
    sincos_lookup(-angle, &s, &c);
    M = (s32 *)(uintptr_t)MEMU32(DH_MATRIX_STACK_CURSOR_ADDR);

    for (r = 0; r < 3; r++) {
        s32 m1 = M[1], m2 = M[2];
        M[2] = (s32)(((long long)(-s) * m1 + (long long)c * m2) >> 16);
        M[1] = (s32)(((long long)c * m1 + (long long)s * m2) >> 16);
        M += 4;
    }
}

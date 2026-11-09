/* 0x02213c4c  mtx_rotate_z: rotate the active 3x4 matrix (M = *g_06098878, 3 rows of
 * 4 longs) about its column-2 axis by `angle`, mixing columns 0 and 1 of every row.
 * angle == 0 is a no-op (shared rts @0x213ca0). Uses sincos_lookup(-angle) (0x213f90).
 * Grouped by fn.units under 0x213ba0 (separate callable entry; counted as an extra).
 * stack 2x2 built as [c,s,-s,c] here (neg into r0 before the pushes).
 * per row:  m0=M[0], m1=M[1];
 *   M[0] = (c*m0 + s*m1) >> 16;  M[1] = (-s*m0 + c*m1) >> 16    (both from originals)
 * fidelity: HIGH - sincos via 0x213f90 on the negated angle; clrmac/mac.l 2-term/
 *   xtrct(>>16) per column, dt r1 (3 rows) loop, 16-byte stride. */
#include "dh_band21.h"

void sincos_lookup(s32 angle, s32 *out_sin, s32 *out_cos);

void mtx_rotate_z(s32 angle) {
    s32 *M;
    s32 s, c;
    int r;

    if (angle == 0) return;
    sincos_lookup(-angle, &s, &c);
    M = (s32 *)(uintptr_t)MEMU32(DH_MATRIX_STACK_CURSOR_ADDR);

    for (r = 0; r < 3; r++) {
        s32 m0 = M[0], m1 = M[1];
        M[0] = (s32)(((long long)c * m0 + (long long)s * m1) >> 16);
        M[1] = (s32)(((long long)(-s) * m0 + (long long)c * m1) >> 16);
        M += 4;
    }
}

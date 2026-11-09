/* matrix_translate_current [0x02213ca4]: Adds the transformed input vector to each active matrix row translation. */
#include "dh.h"
/* 0x02213ca4 (vec3_array_matmul) - transform input vector (x,y,z) by the 3-row matrix
 * pointed to by *g_matrix_stack_cursor (each row = 3 coefficient longs + 1 offset long = 16 bytes).
 * For each row i: result = (dot(row.coef, invec) >> 16) + row.offset, written back into
 * the row's offset slot (in-place). The input vector is also copied to g_06098B9C[0..2].
 * SH-2: clrmac + 3x mac.l per row (s64 MAC) + xtrct (>>16); loop dt r4 x3.
 * fidelity: HIGH - mac.l modeled as explicit s64 accumulator; offset read before the
 * in-place store (same address, read-before-write preserved). */
extern s32 *g_matrix_stack_cursor;     /* -> transform matrix (3 rows x 4 longs) */
extern s32 g_06098B9C[3];   /* input-vector scratch copy              */

void matrix_translate_current(s32 x, s32 y, s32 z) {
    s32 in[3];
    s32 *m, *out;
    s32 i;

    in[0] = x; in[1] = y; in[2] = z;
    /* mov.l @-r3 copies the vector in descending address order. */
    g_06098B9C[2] = z;
    g_06098B9C[1] = y;
    g_06098B9C[0] = x;

    m = g_matrix_stack_cursor;
    out = m + 3;
    for (i = 0; i < 3; i++) {
        s64 acc = (s64)m[0] * in[0] + (s64)m[1] * in[1] + (s64)m[2] * in[2];
        s32 r = (s32)(acc >> 16) + m[3];
        *out = r;
        m += 4;
        out += 4;
    }
}

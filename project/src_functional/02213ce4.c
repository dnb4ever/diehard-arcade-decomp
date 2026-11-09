/* matrix_scale_columns [0x02213ce4]: Scales the active matrix coefficient columns without changing translation. */
#include "dh.h"
/* 0x02213ce4 (mat3_scale_columns) - in-place element-wise scale of the 3x3 coefficient
 * block of matrix *g_06098878 by the input vector (x,y,z) broadcast per column:
 *   m[row][col] = (m[row][col] * in[col]) >> 16     (row,col in 0..2)
 * Each row is 4 longs; the 4th (offset) long is left untouched.
 * fidelity: HIGH - single mac.l per element modeled as s64 product; read-before-write. */
extern s32 *g_matrix_stack_cursor;

void matrix_scale_columns(s32 x, s32 y, s32 z) {
    s32 in[3];
    s32 *m = g_matrix_stack_cursor;
    s32 row, col;

    in[0] = x; in[1] = y; in[2] = z;
    for (row = 0; row < 3; row++) {
        for (col = 0; col < 3; col++) {
            s64 acc = (s64)m[col] * in[col];
            m[col] = (s32)(acc >> 16);
        }
        m += 4;
    }
}

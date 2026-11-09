/* matrix_transform_to_scratch [0x02213538]: Transforms three coordinates through the active matrix into the shared vector scratch. */
#include "dh.h"
/* 0x02213538 (vec3_matmul_out) - transform input (x,y,z) by matrix *g_06098878
 * (3 rows x 4 longs: 3 coeffs + offset), writing the 3 results contiguously to
 * g_06098B1C. result[i] = (dot(row.coef, in) >> 16) + row.offset.
 * fidelity: HIGH - mac.l modeled as s64 accumulator (clrmac/3x mac.l/xtrct). */
extern s32 *g_matrix_stack_cursor;
extern s32 g_transformed_vector_legacy[];

void matrix_transform_to_scratch(s32 x, s32 y, s32 z) {
    s32 in[3];
    s32 *m = g_matrix_stack_cursor;
    s32 *out = g_transformed_vector_legacy;
    s32 i;

    in[0] = x; in[1] = y; in[2] = z;
    for (i = 0; i < 3; i++) {
        s64 acc = (s64)m[0] * in[0] + (s64)m[1] * in[1] + (s64)m[2] * in[2];
        *out++ = (s32)(acc >> 16) + m[3];
        m += 4;
    }
}

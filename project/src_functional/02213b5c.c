/* matrix_transform_translation [0x02213b5c]: Transforms stack matrix translation column into 06098b1c scratch vector. */
#include "dh.h"
/* 0x02213b5c (vec3_matmul_translation) - take the translation column (indices 3,7,11)
 * of the matrix *g_06098878 as the input vector, transform it by the matrix at
 * g_06098AEC (3 rows x 4 longs), and write the 3 results to g_06098B1C.
 * result[i] = (dot(row.coef, in) >> 16) + row.offset.
 * fidelity: HIGH - mac.l modeled as s64 accumulator. */
extern s32 *g_matrix_stack_cursor;
extern s32 g_06098AEC[];
extern s32 g_transformed_vector_legacy[];

void matrix_transform_translation(void) {
    s32 *src = g_matrix_stack_cursor;
    s32 in[3];
    s32 *m = g_06098AEC;
    s32 *out = g_transformed_vector_legacy;
    s32 i;

    in[0] = src[3];
    in[1] = src[7];
    in[2] = src[11];
    for (i = 0; i < 3; i++) {
        s64 acc = (s64)m[0] * in[0] + (s64)m[1] * in[1] + (s64)m[2] * in[2];
        *out++ = (s32)(acc >> 16) + m[3];
        m += 4;
    }
}

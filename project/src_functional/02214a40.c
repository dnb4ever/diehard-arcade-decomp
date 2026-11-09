/* mtx_apply_vec_at8 [0x02214a40]: Translates the active matrix using the world position at object offsets eight through sixteen. */
/* 0x02214a40  mtx_apply_vec_at8: transform the vector at obj+8/+12/+16 by the active
 * matrix in place (vec3_array_matmul):
 *   func_02213ca4(*(s32*)(obj+8), *(s32*)(obj+12), *(s32*)(obj+16));   (tail)
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). mov.l loads @(8/12/16,r14)
 *   -> r4/r5/r6; braf delta 0xfffff254 resolves to 0x213ca4 = vec3_array_matmul. */
#include "dh_band21.h"

void matrix_translate_current(s32 x, s32 y, s32 z);          /* vec3_array_matmul (in place) */

void mtx_apply_vec_at8(u8 *obj) {
    matrix_translate_current(*(s32 *)(obj + 8), *(s32 *)(obj + 12), *(s32 *)(obj + 16));
}

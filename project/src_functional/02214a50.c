/* 0x02214a50  mtx_apply_vec_at0: transform the vector at obj+0/+4/+8 by the active matrix
 * in place (vec3_array_matmul):
 *   func_02213ca4(*(s32*)(obj+0), *(s32*)(obj+4), *(s32*)(obj+8));   (tail)
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). mov.l loads @r14 / @(4,r14)
 *   / @(8,r14); braf delta 0xfffff244 resolves to 0x213ca4 = vec3_array_matmul. */
#include "dh_band21.h"

void matrix_translate_current(s32 x, s32 y, s32 z);          /* vec3_array_matmul (in place) */

void mtx_apply_vec_at0(u8 *obj) {
    matrix_translate_current(*(s32 *)(obj + 0), *(s32 *)(obj + 4), *(s32 *)(obj + 8));
}

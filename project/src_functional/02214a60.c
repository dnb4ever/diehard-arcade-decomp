/* 0x02214a60  mtx_apply_angles_hi: transform a vector built from three s16 angles at obj+0
 * /+2/+4, each shifted left 8, by the active matrix in place (vec3_array_matmul):
 *   func_02213ca4((s16)obj[0]<<8, (s16)obj@2<<8, (s16)obj@4<<8);   (tail)
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). mov.w sign-extends then
 *   shll8; braf delta 0xfffff22a resolves to 0x213ca4 = vec3_array_matmul. */
#include "dh_band21.h"

void matrix_translate_current(s32 x, s32 y, s32 z);          /* vec3_array_matmul (in place) */

void mtx_apply_angles_hi(u8 *obj) {
    matrix_translate_current((*(s16 *)(obj + 0)) << 8,
                  (*(s16 *)(obj + 2)) << 8,
                  (*(s16 *)(obj + 4)) << 8);
}

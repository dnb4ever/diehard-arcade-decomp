/* 0x0221080a  mtx_push_rotate_apply: push the matrix stack, load the default matrix,
 * apply a 3-angle rotation (from `angles`), transform the vector `vec` by the active
 * matrix (-> g_06098B1C), then pop the stack.
 *   *g_06098878 += 0x30;                       (push: advance active-matrix ptr 48 bytes)
 *   load_matrix_default();                     (load_matrix_default - fill new top w/ default)
 *   mtx_rotate_zyx(angles);                    (mtx_rotate_zyx)
 *   func_02213538(vec[0], vec[1], vec[2]);     (mtx_apply_vec -> g_06098B1C)
 *   *g_06098878 -= 0x30;                        (pop)
 * fidelity: HIGH - both deferred callee sigs now pinned: load_matrix_default takes no args
 *   (48-byte matrix template copy), mtx_rotate_zyx takes the angle-struct ptr (r4=arg1,
 *   reloaded from the 4-byte stack slot). vec reads are mov.l @r13 / @(4,r13) / @(8,r13).
 *   bsrf deltas verified: 0x213fb4, 0x2149e4, 0x213538. */
#include "dh_band21.h"

void load_matrix_default(void);                   /* 0x02213fb4 */
void mtx_rotate_zyx(u16 *angles);                 /* 0x022149e4 */
void matrix_transform_to_scratch(s32 x, s32 y, s32 z);          /* mtx_apply_vec -> g_06098B1C */

void mtx_push_rotate_apply(s32 *vec, u16 *angles) {
    MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    load_matrix_default();
    mtx_rotate_zyx(angles);
    matrix_transform_to_scratch(vec[0], vec[1], vec[2]);
    MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

/* mtx_build_fwd_inv [0x02214a7a]: Builds and caches forward and inverse transforms from the world pose. */
/* 0x02214a7a  mtx_build_fwd_inv: push the matrix stack and build both the forward and the
 * inverse model transform for the object `obj`, caching them at g_06098aec (forward) and
 * g_06098abc (inverse), then pop.
 *   *g_06098878 += 0x30;                                   (push)
 *   load_matrix_default();
 *   func_02213ca4(obj[8], obj[12], obj[16]);               (apply translate, vec3_array_matmul)
 *   mtx_rotate_zyx_at24(obj);                              (rotate Z@28,Y@26,X@24)
 *   active = *g_06098878; copy_longs(active, g_06098aec, 12);   (save forward)
 *   load_matrix_default();
 *   mtx_rotate_x(-(u16)obj@24); mtx_rotate_y(-(u16)obj@26); mtx_rotate_z(-(u16)obj@28);
 *   func_02213ca4(-obj[8], -obj[12], -obj[16]);
 *   copy_longs(active, g_06098abc, 12);                    (save inverse)
 *   *g_06098878 -= 0x30;                                   (pop)
 * fidelity: HIGH (extra - sibling of fn-unit member 0x2149a0). All 8 bsrf targets resolve
 *   (0x213fb4, 0x213ca4, 0x213ba0/bf4/c4c, 0x21326c) + one bsr to 0x214a06; inverse angles
 *   are extu.w then neg; inverse rotations run X,Y,Z (opposite order) negated. `active`
 *   points at the pushed top; load_matrix_default rewrites that same slot before the
 *   inverse pass, so the second copy_longs reads the rotated active matrix. */
#include "dh_band21.h"

void load_matrix_default(void);                   /* 0x02213fb4 */
void copy_longs(s32 *src, s32 *dst, s32 count);   /* 0x0221326c */
void matrix_translate_current(s32 x, s32 y, s32 z);          /* vec3_array_matmul (in place) */
void mtx_rotate_x(s32 angle);                     /* 0x02213ba0 */
void mtx_rotate_y(s32 angle);                     /* 0x02213bf4 */
void mtx_rotate_z(s32 angle);                     /* 0x02213c4c */
void mtx_rotate_zyx_at24(u8 *obj);                /* 0x02214a06 */

void mtx_build_fwd_inv(u8 *obj) {
    s32 *active;

    MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    load_matrix_default();
    matrix_translate_current(*(s32 *)(obj + 8), *(s32 *)(obj + 12), *(s32 *)(obj + 16));
    mtx_rotate_zyx_at24(obj);

    active = (s32 *)(uintptr_t)MEMU32(DH_MATRIX_STACK_CURSOR_ADDR);
    copy_longs(active, (s32 *)0x06098aec, 12);

    load_matrix_default();
    mtx_rotate_x(-(s32)*(u16 *)(obj + 0x24));
    mtx_rotate_y(-(s32)*(u16 *)(obj + 0x26));
    mtx_rotate_z(-(s32)*(u16 *)(obj + 0x28));
    matrix_translate_current(-*(s32 *)(obj + 8), -*(s32 *)(obj + 12), -*(s32 *)(obj + 16));
    copy_longs(active, (s32 *)0x06098abc, 12);

    MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

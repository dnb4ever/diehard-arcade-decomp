/* orientation_compose_angles [0x02210690]: Builds matrix from two angle sets and derives three output angles. */
#include "dh.h"
/* 0x02210690 (orientation-angle builder) - from two angle-sets, derive the three
 * look-at angles of an object and store them in *out (out->unk0, unk2, unk4).
 *
 * Pushes the matrix stack (*g_06098878 += 0x30), builds M = default * Rzyx(A) *
 * Rzyx(B), then:
 *   1. transform +X basis (0x80000,0,0) via mtx_apply_vec -> g_06098b1c, and take
 *      look-at angles of that vector (helper func_02210622): out->unk4 = -heading,
 *      out->unk2 = elevation.
 *   2. transform +Z basis (0,0,0x80000) -> save that vector.
 *   3. reset matrix, rotate_y(-out->unk2), rotate_z(-out->unk4), re-apply to the
 *      saved vector, and out->unk0 = angle(z', y') (roll).
 * Pops the matrix stack (-0x30) and returns.
 *
 * ABI: r4 = angle-set A ptr, r5 = angle-set B ptr (both u16[] for mtx_rotate_zyx),
 * r6 = out ptr (3 s16 angle fields at +0/+2/+4). No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; indirect jsr r11 resolved to
 * mtx_apply_vec (0x02213538) via ptr_resolve; all bsrf targets resolved. */

extern void load_matrix_default(void);                 /* load_matrix_default (no args) */
extern void mtx_rotate_zyx(void *angles);         /* mtx_rotate_zyx(u16 *angles)   */
extern void matrix_transform_to_scratch(s32 x, s32 y, s32 z);  /* mtx_apply_vec -> g_06098b1c   */
extern void mtx_rotate_y(s32 angle);            /* mtx_rotate_y                  */
extern void mtx_rotate_z(s32 angle);            /* mtx_rotate_z                  */
extern s32  atan2_fixed(s32 a, s32 b);         /* atan2-style angle             */
extern void camera_compute_xy_lookat(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5,
                          s16 *a6, s16 *a7);      /* look-at angles of a vector    */

extern s32 g_matrix_stack_cursor;      /* active matrix-stack pointer (incremented by 0x30) */
extern s32 g_transformed_vector[];    /* mtx_apply_vec output vector: [0]=x [1]=y [2]=z     */

void orientation_compose_angles(void *angles_a, void *angles_b, void *out)
{
    s32 vx, vy, vz;

    g_matrix_stack_cursor += 0x30;
    load_matrix_default();
    mtx_rotate_zyx(angles_a);
    mtx_rotate_zyx(angles_b);

    matrix_transform_to_scratch(0x80000, 0, 0);
    camera_compute_xy_lookat(0, 0, 0, g_transformed_vector[0], g_transformed_vector[1], g_transformed_vector[2],
                  (s16 *)((u8 *)out + 4), (s16 *)((u8 *)out + 2));

    matrix_transform_to_scratch(0, 0, 0x80000);
    vx = g_transformed_vector[0];
    vy = g_transformed_vector[1];
    vz = g_transformed_vector[2];

    load_matrix_default();
    mtx_rotate_y(-(s32)(u16)M16(out, 2));
    mtx_rotate_z(-(s32)(u16)M16(out, 4));
    matrix_transform_to_scratch(vx, vy, vz);

    M16(out, 0) = (s16)atan2_fixed(g_transformed_vector[2], g_transformed_vector[1]);

    g_matrix_stack_cursor -= 0x30;
}

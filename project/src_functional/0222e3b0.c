/* pose_blend_rotation_matrices [0x0222e3b0]: Sixteen joints blend previous/next matrices then decompose Euler angles; animation caller. */
#include "dh.h"
/* 0x0222e3b0  pose_matrix_blend_decompose
 *
 * Blend two per-joint rotation matrices (previous-pose and next-pose Euler
 * angle sets) by a fixed-point weight, then re-decompose the blended basis
 * back into three Euler-angle shorts, for each of 16 joints.  bsr'd by
 * actor_anim_frame_update (0x0222dc2c) on its SHBASE!=0 interpolation path.
 *   arg0 = joint-table index u34   (buffer offset = (s16)(arg0 * 108))
 *   arg1 = fixed-point shift bits  (var_r12 = SHBASE + record field)
 *   arg2 = blend weight w          ((u8)entity->unk40)   returns void.
 *
 * Working angle buffers (stride 108, off = (s16)(arg0*108), 6 bytes/joint):
 *   prev @0x0609661C   next @0x06096910   cur @0x06096328 (output angles).
 * The active matrix cell is *0x06098878 (push +0x30 on entry, pop -0x30 on
 * exit).  Each joint:
 *   - load_matrix_default + mtx_rotate_zyx(prev)  -> matrix A; grab basis
 *     columns 0 and 2  (A0={M00,M10,M20}, A1={M02,M12,M22}),
 *   - the same from next-pose angles             -> matrix B columns 0,2,
 *   - blend each column   col = A*w + B*(1-w),    1.0 == (1<<arg1),
 *   - lookat_angles_xy((0,0,0)->blend0) writes the Y/Z angles to cur[+2],
 *     cur[+4]; rebuild M = rot_y(-cur[+2]) . rot_z(-cur[+4]), push blend1
 *     through it (mtx_apply_vec -> g_06098B1C), and the X angle is
 *     angle_from_vec2(dvec[2], dvec[1]) stored to cur[+0].
 *
 * --- stack-frame map (why m2c choked, and why this is a rolled loop) ---
 * Prologue saves r8-r14,pr,macl (9 longs) then `add #-104,r15`; call the post-
 * prologue sp SP0.  The 104-byte local frame (offsets rel. SP0):
 *   +0x00 ->&(+0x2C)  +0x04 ->&(+0x20)  +0x08 ->&(+0x44)   (blend/matB ptrs)
 *   +0x0C prev ptr    +0x10 next ptr    +0x14 loop count(16)
 *   +0x18 =0x06098B24 +0x1C =0x06098B20 (angle_from_vec2 arg ptrs)
 *   +0x20..+0x2B blend0   +0x2C..+0x37 blend1   +0x38..+0x43 matB0
 *   +0x44..+0x4F matB1    +0x50..+0x5B matA0    +0x5C..+0x67 matA1
 * The loop is 4x unrolled.  Each unrolled block marshals lookat_angles_xy's
 * four stack args with `mov.l rN,@-r15`; these four pushes are NOT cleaned per
 * block, so sp slides SP0 -> SP0-16 -> SP0-32 -> SP0-48 across the four blocks
 * and is bulk-restored by `add #64,r15` (0x22e9a2) just before the last block
 * finishes, then `add #104,r15` in the epilogue.  Every @(disp,r15) inside a
 * block carries a +0/+16/+32/+48 compensation, so all four blocks address the
 * SAME frame slots and scratch vec3s above -- they are behaviourally identical
 * and the scratch is fully overwritten each joint (no cross-joint carry except
 * the three monotonic +6 pointer bumps and the counter).  m2c's fixed-slot
 * model can't track the sliding sp and emits undefined sp24/sp28/sp38; modelling
 * it instead as the equivalent 16-iteration rolled loop with named locals is
 * exact and avoids the hazard entirely.
 *
 * All 41 computed branches resolved (braf_resolve.py); the lone variable shift
 * (1<<arg1) is routed through func_0224a360 (ashl_var) which m2c drops.
 * Callee ABIs cross-checked against symbols.txt and siblings 0222dc2c/0222d0e8.
 * Compiles -Wall -Wextra clean.
 */
extern s32  shc_shift_left(s32 value, s32 count);   /* ashl_var: value<<count   */
extern void load_matrix_default(void);                   /* load_matrix_default      */
extern void mtx_rotate_zyx(void *angles);           /* mtx_rotate_zyx (u16[3])  */
extern void mtx_rotate_y(s32 angle);              /* mtx_rotate_y             */
extern void mtx_rotate_z(s32 angle);              /* mtx_rotate_z             */
extern void matrix_transform_to_scratch(s32 x, s32 y, s32 z);    /* mtx_apply_vec -> B1C     */
extern s32  atan2_fixed(s32 a, s32 b);           /* angle_from_vec2 (atan2)  */
extern void camera_compute_xy_lookat(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5,
                          void *out_y, void *out_z); /* lookat_angles_xy       */

#define MTX   M32((void *)DH_MATRIX_STACK_CURSOR_ADDR_U, 0)   /* active matrix-stack top pointer */
#define DVEC  ((void *)DH_TRANSFORMED_VECTOR_ADDR_U)         /* mtx_apply_vec output vector     */

void pose_blend_rotation_matrices(s32 arg0, s32 arg1, s32 arg2) {
    s32  off  = (s16)(arg0 * 108);
    u8  *prev = (u8 *)(uintptr_t)(0x0609661Cu + off);   /* prev-pose angles */
    u8  *next = (u8 *)(uintptr_t)(0x06096910u + off);   /* next-pose angles */
    u8  *cur  = (u8 *)(uintptr_t)(0x06096328u + off);   /* output angles    */
    s32  w    = arg2;                           /* blend weight (fixed point)  */
    s32  b    = shc_shift_left(1, arg1) - arg2;  /* (1<<arg1) - w  == 1.0 - w   */
    s32  i;

    MTX += 0x30;                                /* push a working matrix cell  */

    for (i = 0; i < 16; i++) {
        s32 a0[3], a1[3];     /* prev-pose basis columns 0 and 2 */
        s32 bb0[3], bb1[3];   /* next-pose basis columns 0 and 2 */
        s32 bl0[3], bl1[3];   /* weighted blend of each column   */
        u8 *m;

        /* rotation matrix from the previous-pose Euler angles */
        load_matrix_default();
        mtx_rotate_zyx(prev);
        m = (u8 *)(uintptr_t)MTX;
        a0[0] = M32(m, 0x00); a0[1] = M32(m, 0x10); a0[2] = M32(m, 0x20);
        a1[0] = M32(m, 0x08); a1[1] = M32(m, 0x18); a1[2] = M32(m, 0x28);

        /* rotation matrix from the next-pose Euler angles */
        load_matrix_default();
        mtx_rotate_zyx(next);
        m = (u8 *)(uintptr_t)MTX;
        bb0[0] = M32(m, 0x00); bb0[1] = M32(m, 0x10); bb0[2] = M32(m, 0x20);
        bb1[0] = M32(m, 0x08); bb1[1] = M32(m, 0x18); bb1[2] = M32(m, 0x28);

        /* blend the two basis columns: col*w + col*(1-w) */
        bl0[0] = a0[0] * w + bb0[0] * b;
        bl0[1] = a0[1] * w + bb0[1] * b;
        bl0[2] = a0[2] * w + bb0[2] * b;
        bl1[0] = a1[0] * w + bb1[0] * b;
        bl1[1] = a1[1] * w + bb1[1] * b;
        bl1[2] = a1[2] * w + bb1[2] * b;

        /* re-decompose the blended basis back into three Euler-angle shorts */
        camera_compute_xy_lookat(0, 0, 0, bl0[0], bl0[1], bl0[2],
                      cur + 4, cur + 2);               /* writes cur[+2],cur[+4] */
        load_matrix_default();
        mtx_rotate_y(-(s32)(u16)M16(cur, 2));
        mtx_rotate_z(-(s32)(u16)M16(cur, 4));
        matrix_transform_to_scratch(bl1[0], bl1[1], bl1[2]);         /* -> g_06098B1C */
        M16(cur, 0) = (s16)atan2_fixed(M32(DVEC, 8), M32(DVEC, 4));

        prev += 6;
        next += 6;
        cur  += 6;
    }

    MTX -= 0x30;                                /* pop the working matrix cell */
}

/* camera_integrate_local_velocity [0x0220ebde]: Rotates the camera's own velocity into its current facing and advances position. */
#include "dh.h"
/* 0x0220ebde - camera-mode service, self-propelled variant (sibling of 0x0220ea06 /
 * 0x0220ef02 / 0x0220f28e). Same mode dispatch off g_06099584 (mode = unk10 + unk14;
 * 0 no-op, 1/2 single source, 3 dual source), but this camera carries its own velocity
 * and advances its world position each frame by that velocity rotated into the current
 * facing.
 *
 *   mode 1 : src  = (*g_06099584->unk30)->unk8                              kind = 1
 *   mode 2 : src  = (*g_06099584->unk34)->unk8                              kind = 1
 *   mode 3 : src  = (*g_06099584->unk30)->unk8,
 *            src2 = (*g_06099584->unk34)->unk8                              kind = 2
 *
 * First frame (unk68 == 1): seed world pos unk8/C/10 from arg1; seed velocity
 * unk14/18/1C = arg2 >> g_06096C90 (arithmetic, func_0224a420/ashr_var); seed the camera
 * triple(s) from the source(s) and run the transform func_0221055c once from the primary
 * source (by = src->Y + 0x15000).
 *
 * Steady state: ease the camera triple(s) a quarter toward the source(s) (delta/4 via
 * sdiv32); rotate the velocity unk14/18/1C by the facing unk24/26 into g_06098b1c
 * (mtx_push_rotate_apply, mtx_push_rotate_apply); advance world pos unk8/C/10 += g_06098b1c; then
 * rebuild the look-at via func_0221055c (kind 1: bx=unk90, bz=unk98; kind 2: bx/bz are the
 * midpoints of the two tracked triples; by is always unk94 + 0x15000).
 *
 * ABI / branches: func_0224a420 (ashr_var, r0/r1), sdiv32 (0x0224a06c, r1/r0),
 * mtx_push_rotate_apply (mtx_push_rotate_apply), and the transform func_0221055c (reached via
 * mova+0x18FC jsr @r8 = 0x20ec60+0x18FC = 0x21055c lookat_angles_xz) resolved by hand.
 * Every "TABLE/COMPUTED" braf_resolve reports in
 * this function (0x20ec30.., 0x20ed34.., 0x20edec..) is a PHANTOM literal-pool long
 * (0x0003xxxx == `bsrf r0`): these are the neighbouring ea06/ebde/ef02 pools physically
 * interleaved into this address range, not branches. The /2 midpoints use the SH-2
 * addc/shar round-toward-zero idiom == C signed /2. fidelity: HIGH - verified vs asm. */
extern s32  shc_shift_right_signed(s32 value, s32 count);                               /* ashr_var 0x0224a420 */
extern s32  sdiv32(s32 dividend, s32 divisor);                                 /* 0x0224a06c */
extern void mtx_push_rotate_apply(s32 *vec, u16 *angles);                              /* mtx_push_rotate_apply -> g_06098b1c */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *a6, s16 *a7);                                    /* lookat-shaped transform */
extern u8   g_actor_slot_registry[];                                                      /* camera control block */
extern s32  g_06096C90;                                                        /* velocity shift count */
extern s32  g_transformed_vector[];                                                      /* mtx-apply output vector */

void camera_integrate_local_velocity(void *arg0, void *arg1, void *arg2) {
    void *src  = (void *)0;
    void *src2 = (void *)0;
    s32 kind;
    s32 mode = M32(g_actor_slot_registry, 0x10) + M32(g_actor_slot_registry, 0x14);

    switch (mode) {
    case 0:
        return;
    case 1:
        src  = MP(MP(g_actor_slot_registry, 0x30), 8);
        kind = 1;
        break;
    case 2:
        src  = MP(MP(g_actor_slot_registry, 0x34), 8);
        kind = 1;
        break;
    case 3:
        src  = MP(MP(g_actor_slot_registry, 0x30), 8);
        src2 = MP(MP(g_actor_slot_registry, 0x34), 8);
        kind = 2;
        break;
    default:
        kind = mode;
        break;
    }

    if (M32(arg0, 0x68) == 1) {                 /* first-frame seed */
        s32 cnt = g_06096C90;
        M32(arg0, 0x08) = M32(arg1, 0x00);
        M32(arg0, 0x0C) = M32(arg1, 0x04);
        M32(arg0, 0x10) = M32(arg1, 0x08);
        M32(arg0, 0x14) = shc_shift_right_signed(M32(arg2, 0x00), cnt);
        M32(arg0, 0x18) = shc_shift_right_signed(M32(arg2, 0x04), cnt);
        M32(arg0, 0x1C) = shc_shift_right_signed(M32(arg2, 0x08), cnt);
        M32(arg0, 0x68) = 0;
        if (kind == 1) {
            M32(arg0, 0x90) = M32(src, 0x08);
            M32(arg0, 0x94) = M32(src, 0x0C);
            M32(arg0, 0x98) = M32(src, 0x10);
            camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                          M32(src, 0x08), M32(src, 0x0C) + 0x15000, M32(src, 0x10),
                          (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
        }
        if (kind != 2) {
            return;
        }
        M32(arg0, 0x90) = M32(src,  0x08);
        M32(arg0, 0x94) = M32(src,  0x0C);
        M32(arg0, 0x98) = M32(src,  0x10);
        M32(arg0, 0x9C) = M32(src2, 0x08);
        M32(arg0, 0xA0) = M32(src2, 0x0C);
        M32(arg0, 0xA4) = M32(src2, 0x10);
        camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                      M32(src, 0x08), M32(src, 0x0C) + 0x15000, M32(src, 0x10),
                      (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
        return;
    }

    if (kind == 1) {
        M32(arg0, 0x90) += sdiv32(M32(src, 0x08) - M32(arg0, 0x90), 4);
        M32(arg0, 0x94) += sdiv32(M32(src, 0x0C) - M32(arg0, 0x94), 4);
        M32(arg0, 0x98) += sdiv32(M32(src, 0x10) - M32(arg0, 0x98), 4);
        mtx_push_rotate_apply((s32 *)((u8 *)arg0 + 0x14), (u16 *)((u8 *)arg0 + 0x24));
        M32(arg0, 0x08) += g_transformed_vector[0];
        M32(arg0, 0x0C) += g_transformed_vector[1];
        M32(arg0, 0x10) += g_transformed_vector[2];
        camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                      M32(arg0, 0x90), M32(arg0, 0x94) + 0x15000, M32(arg0, 0x98),
                      (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
        return;
    }

    if (kind == 2) {
        M32(arg0, 0x90) += sdiv32(M32(src,  0x08) - M32(arg0, 0x90), 4);
        M32(arg0, 0x94) += sdiv32(M32(src,  0x0C) - M32(arg0, 0x94), 4);
        M32(arg0, 0x98) += sdiv32(M32(src,  0x10) - M32(arg0, 0x98), 4);
        M32(arg0, 0x9C) += sdiv32(M32(src2, 0x08) - M32(arg0, 0x9C), 4);
        M32(arg0, 0xA0) += sdiv32(M32(src2, 0x0C) - M32(arg0, 0xA0), 4);
        M32(arg0, 0xA4) += sdiv32(M32(src2, 0x10) - M32(arg0, 0xA4), 4);
        mtx_push_rotate_apply((s32 *)((u8 *)arg0 + 0x14), (u16 *)((u8 *)arg0 + 0x24));
        M32(arg0, 0x08) += g_transformed_vector[0];
        M32(arg0, 0x0C) += g_transformed_vector[1];
        M32(arg0, 0x10) += g_transformed_vector[2];
        camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                      (M32(arg0, 0x90) + M32(arg0, 0x9C)) / 2, M32(arg0, 0x94) + 0x15000,
                      (M32(arg0, 0x98) + M32(arg0, 0xA4)) / 2,
                      (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
        return;
    }
}

/* camera_integrate_velocity_pose [0x0220e6d4]: Seeds shifted velocity and offset vectors and advances the moving look-at pose. */
#include "dh.h"
/* 0x0220e6d4 - per-frame motion integrator + lookat-angle builder for a moving
 * object (arg0), seeded from a velocity vector arg1 and an offset vector arg2.
 *
 * First frame only (arg0->unk68 == 1): cache both source vectors arithmetic-
 * shifted right by the global shift count at 0x06096c90 via func_0224a420
 * (ashr_var, r0=value r1=count):
 *   unk14/18/1C = arg1->{0,4,8} >> g_06096c90
 *   unk40/44/48 = arg2->{0,4,8} >> g_06096c90
 * then clear the first-frame flag unk68.
 *
 * Every frame: integrate unk8/C/10 += unk14/18/1C and the secondary accumulator
 * unkB8/BC/C0 += unk40/44/48, then build the two view angles into
 * unk24 / unk26 via func_0221055c (lookat_angles_xz) from position
 * (unk8,unkC,unk10) and target (unkB8,unkBC,unkC0).
 *
 * fidelity: HIGH - hand-traced vs the reachable asm (0x20e6d4..0x20e7a2). The
 * six bsrf target func_0224a420 (delay-slot load feeds r0=value, @r4 global
 * feed r1=count, store takes the r0 return); the final bsrf targets
 * func_0221055c with bx/by/bz = unkB8/BC/C0 per the r0=0xB8 pointer walk at
 * 0x20e74e-0x20e78e and the push order at 0x20e77c-0x20e78c. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern s32  shc_shift_right_signed(s32 value, s32 count);                 /* ashr_var 0x0224a420 */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *a7, s16 *a8);                      /* lookat_angles_xz 0x0221055c */

void camera_integrate_velocity_pose(void *arg0, void *arg1, void *arg2) {
    if (M32(arg0, 0x68) == 1) {
        s32 cnt = G32(0x06096c90);
        M32(arg0, 0x14) = shc_shift_right_signed(M32(arg1, 0x00), cnt);
        M32(arg0, 0x18) = shc_shift_right_signed(M32(arg1, 0x04), cnt);
        M32(arg0, 0x1C) = shc_shift_right_signed(M32(arg1, 0x08), cnt);
        M32(arg0, 0x40) = shc_shift_right_signed(M32(arg2, 0x00), cnt);
        M32(arg0, 0x44) = shc_shift_right_signed(M32(arg2, 0x04), cnt);
        M32(arg0, 0x48) = shc_shift_right_signed(M32(arg2, 0x08), cnt);
        M32(arg0, 0x68) = 0;
    }

    M32(arg0, 0x08) += M32(arg0, 0x14);
    M32(arg0, 0x0C) += M32(arg0, 0x18);
    M32(arg0, 0x10) += M32(arg0, 0x1C);
    M32(arg0, 0xB8) += M32(arg0, 0x40);
    M32(arg0, 0xBC) += M32(arg0, 0x44);
    M32(arg0, 0xC0) += M32(arg0, 0x48);

    camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                  M32(arg0, 0xB8), M32(arg0, 0xBC), M32(arg0, 0xC0),
                  (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
}

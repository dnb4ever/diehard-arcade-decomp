/* camera_integrate_seeded_pose [0x0220e5bc]: Seeds position and velocity vectors before integrating motion and look-at angles. */
#include "dh.h"
/* func_0220e5bc (cart 0x20e5bc) - per-frame motion integrator + lookat-angle
 * builder, seeding variant of func_0220e6d4.
 *
 * ABI (self, posVec, tgtVec) = (r4, r5, r6); the 3-vectors are {x@0, y@4, z@8}
 * plus a second triple at +0xC..0x14.
 *
 * First frame only (self->unk68 == 1):
 *   self->unk8/C/10 = posVec->{0,4,8}                      (direct copy)
 *   self->unk14/18/1C = posVec->{C,10,14} >> g_06096c90     (func_0224a420 ashr)
 *   self->unkB8/BC/C0 = tgtVec->{0,4,8}                    (direct copy)
 *   self->unk40/44/48 = tgtVec->{C,10,14} >> g_06096c90
 *   self->unk68 = 0
 * Every frame:
 *   self->unk8/C/10 += self->unk14/18/1C
 *   self->unkB8/BC/C0 += self->unk40/44/48
 *   lookat func_0221055c(unk8,unkC,unk10, unkB8,unkBC,unkC0, &unk24, &unk26)
 * fidelity: HIGH - hand-traced; ashr value comes from the bsrf delay-slot load,
 * stored result is the func_0224a420 return (r0). */

#define G32(a) (*(s32 *)(uintptr_t)(a))

extern s32  shc_shift_right_signed(s32 value, s32 count);   /* ashr_var 0x0224a420 */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *a6, s16 *a7);        /* lookat_angles_xz 0x0221055c */

void camera_integrate_seeded_pose(void *self, void *posVec, void *tgtVec)
{
	if (M32(self, 0x68) == 1) {
		s32 cnt = G32(0x06096c90);

		M32(self, 0x8)  = M32(posVec, 0x0);
		M32(self, 0xC)  = M32(posVec, 0x4);
		M32(self, 0x10) = M32(posVec, 0x8);
		M32(self, 0x14) = shc_shift_right_signed(M32(posVec, 0xC), cnt);
		M32(self, 0x18) = shc_shift_right_signed(M32(posVec, 0x10), cnt);
		M32(self, 0x1C) = shc_shift_right_signed(M32(posVec, 0x14), cnt);

		M32(self, 0xB8) = M32(tgtVec, 0x0);
		M32(self, 0xBC) = M32(tgtVec, 0x4);
		M32(self, 0xC0) = M32(tgtVec, 0x8);
		M32(self, 0x40) = shc_shift_right_signed(M32(tgtVec, 0xC), cnt);
		M32(self, 0x44) = shc_shift_right_signed(M32(tgtVec, 0x10), cnt);
		M32(self, 0x48) = shc_shift_right_signed(M32(tgtVec, 0x14), cnt);

		M32(self, 0x68) = 0;
	} else {
		M32(self, 0x8)  += M32(self, 0x14);
		M32(self, 0xC)  += M32(self, 0x18);
		M32(self, 0x10) += M32(self, 0x1C);
		M32(self, 0xB8) += M32(self, 0x40);
		M32(self, 0xBC) += M32(self, 0x44);
		M32(self, 0xC0) += M32(self, 0x48);
	}

	camera_compute_lookat_angles(M32(self, 0x8), M32(self, 0xC), M32(self, 0x10),
	              M32(self, 0xB8), M32(self, 0xBC), M32(self, 0xC0),
	                  (s16 *)((u8 *)self + 0x24), (s16 *)((u8 *)self + 0x26));
}

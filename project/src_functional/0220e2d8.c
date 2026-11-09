/* debug_orbit_camera_task [0x0220e2d8]: Reads direction/action controls to change orbit radius, angle and offsets. */
#include "dh.h"

/* func_0220e2d8 (cart 0x20e2d8) - free-look / orbit-camera controller task.
 * self = task (r4); entity = *(void**)(self+8). State byte at self+0x10
 * (0 = init, 1 = run, anything else = skip straight to the matrix rebuild).
 *
 * On init the orbit state is seeded (radius unk64 = 0x30000, angle unk68 = 0,
 * the three local offset accumulators unk6C/70/78 = 0 with unk74 = 0x12000,
 * the base/position coords, and the toggle flag unkCA = 1). Each run frame,
 * gated by g_06099594, it reads two input words:
 *   - g_06098750 ("d-pad" word): bits 1&2&4 together re-seed the orbit;
 *     bit 4 also toggles unkCA.
 *   - g_0609874c (mode/button word): low 2 bits select which accumulator the
 *     +/-0x40/0x80/0x100/0x200 d-pad bits drive, but only while the control
 *     enable bit (g_06098794 & 0x20) is set.
 * When the toggle flag or the "moved" flag is set, the base position
 * unkB8/BC/C0 is recomputed from the shared base object (*g_060995b4)->unk8
 * plus the accumulators. Finally the world position is rebuilt from the orbit:
 *   unk8  (X) = fixmul(sin(unk68), unk64) + unkB8;
 *   unkC  (Y) = unkBC + unk6C;
 *   unk10 (Z) = unkC0 - fixmul(cos(unk68), unk64);
 * handed with the base coords and the angle-field pair at +0x24/+0x26 to the
 * look-at builder func_0221055c, then it tail-calls func_02214a7a
 * (mtx_build_fwd_inv) on the entity.
 *
 * BOUNDED: real extent 0x20e2d8..0x20e598, ending at the braf tail-call; m2c
 * over-swallows the following siblings. The "internal computed call" flagged
 * by the raw branch sweep (a bsrf r0 at 0x20e440) is a PHANTOM: 0x20e440 is the
 * inline literal pool word 0x00030000, whose high half 0x0003 mis-decodes as
 * bsrf r0. No computed call exists. The func_0224a420 (ashr) cluster is in the
 * NEXT function (>0x20e598), not here.
 */

extern s32  sin_lookup(s32 angle);                 /* 0x02213190 */
extern s32  cos_lookup(s32 angle);                 /* 0x022131a8 */
extern s32  fixed_mul16(s32 a, s32 b);           /* fixmul_16_16 */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          void *angp, void *angp2); /* lookat_angles_xz */
extern void mtx_build_fwd_inv_entry(void *obj);              /* mtx_build_fwd_inv */

extern u16   g_0609874c;   /* mode / button input word */
extern u16   g_06098750;   /* d-pad input word */
extern u16   g_06098794;   /* control-enable flag word (bit 0x20) */
extern s32   g_06099594;   /* run gate */
extern void *g_060995b4;   /* shared base-object pointer */

void debug_orbit_camera_task(void *self)
{
	void *e = MP(self, 8);
	s32 st = M8(self, 16);
	void *base;
	s32 moved = 0;

	if (st == 0) {
		M32(e, 0x8)  = 0;
		M32(e, 0xC)  = 0x12000;
		M32(e, 0x10) = 0;
		M16(e, 0x24) = 0;
		M16(e, 0x26) = 0;
		M16(e, 0x28) = 0;
		M32(e, 0xB8) = 0;
		M32(e, 0xBC) = 0x12000;
		M32(e, 0xC0) = 0;
		M32(e, 0x64) = 0x30000;
		M32(e, 0x68) = 0;
		M32(e, 0x6C) = 0;
		M32(e, 0x70) = 0;
		M32(e, 0x74) = 0x12000;
		M32(e, 0x78) = 0;
		M8(e, 0xCA)  = 1;
		M8(self, 16) = 1;
	}

	if ((st == 0 || st == 1) && g_06099594 != 0) {
		if ((g_06098750 & DH_PAD_ACTION1) && (g_06098750 & DH_PAD_ACTION2) && (g_06098750 & DH_PAD_ACTION3)) {
			moved = 1;
			M32(e, 0x64) = 0x30000;
			M32(e, 0x68) = 0;
			M32(e, 0x6C) = 0;
			M32(e, 0x70) = 0;
			M32(e, 0x74) = 0x12000;
			M32(e, 0x78) = 0;
		}

		base = MP(g_060995b4, 8);

		if (g_06098794 & 0x20) {
			switch (g_0609874c & 3) {
			case 0:
				if (g_0609874c & DH_PAD_X_POS) M32(e, 0x68) -= 0x200;
				if (g_0609874c & DH_PAD_X_NEG) M32(e, 0x68) += 0x200;
				if (g_0609874c & DH_PAD_Z_POS) {
					M32(e, 0x64) -= 0x2000;
					if (M32(e, 0x64) < 0x1000) M32(e, 0x64) = 0x1000;
				}
				if (g_0609874c & DH_PAD_Z_NEG) M32(e, 0x64) += 0x2000;
				break;
			case 1:
				if (g_0609874c & DH_PAD_Z_POS) M32(e, 0x6C) += 0x2000;
				if (g_0609874c & DH_PAD_Z_NEG) M32(e, 0x6C) -= 0x2000;
				break;
			case 2:
				if (g_0609874c & DH_PAD_Z_POS) { moved = 1; M32(e, 0x74) += 0x2000; }
				if (g_0609874c & DH_PAD_Z_NEG) { M32(e, 0x74) -= 0x2000; moved = 1; }
				break;
			case 3:
				if (g_0609874c & DH_PAD_X_POS) { moved = 1; M32(e, 0x70) -= 0x2000; }
				if (g_0609874c & DH_PAD_X_NEG) { moved = 1; M32(e, 0x70) += 0x2000; }
				if (g_0609874c & DH_PAD_Z_POS)  { moved = 1; M32(e, 0x78) += 0x2000; }
				if (g_0609874c & DH_PAD_Z_NEG)  { moved = 1; M32(e, 0x78) -= 0x2000; }
				break;
			}

			if (g_06098750 & DH_PAD_ACTION3)
				M8(e, 0xCA) = (M8(e, 0xCA) == 0);
		}

		if (M8(e, 0xCA) != 0 || moved != 0) {
			M32(e, 0xB8) = M32(base, 0x8)  + M32(e, 0x70);
			M32(e, 0xBC) = M32(base, 0xC)  + M32(e, 0x74);
			M32(e, 0xC0) = M32(base, 0x10) + M32(e, 0x78);
		}

		M32(e, 0x8)  = fixed_mul16(sin_lookup(M32(e, 0x68)), M32(e, 0x64)) + M32(e, 0xB8);
		M32(e, 0xC)  = M32(e, 0xBC) + M32(e, 0x6C);
		M32(e, 0x10) = M32(e, 0xC0) - fixed_mul16(cos_lookup(M32(e, 0x68)), M32(e, 0x64));

		camera_compute_lookat_angles(M32(e, 0x8), M32(e, 0xC), M32(e, 0x10),
		              M32(e, 0xB8), M32(e, 0xBC), M32(e, 0xC0),
		              (char *)e + 0x24, (char *)e + 0x26);

		M16(e, 0x28) = 0;
	}

	mtx_build_fwd_inv_entry(e);
}
/* fidelity: HIGH - hand-traced vs --asm, bounded 0x20e2d8..0x20e598; calls resolved via pool_read --branches (sin 0x213190 / cos 0x2131a8 / fixmul 0x2131c8 / lookat 0x21055c / tail mtx_build_fwd_inv 0x214a7a); the raw-sweep "bsrf r0 @0x20e440" is a phantom (inline pool long 0x00030000). */

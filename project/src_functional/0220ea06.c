/* camera_ease_source_pose [0x0220ea06]: Selects one or two actor sources and eases the camera pose toward them. */
#include "dh.h"
/* 0x0220ea06 - camera-mode service (sibling of 0x0220f28e / 0x0220f454 slew family).
 * Selects a camera source from the control block g_06099584 by a mode index
 * (mode = g_06099584->unk10 + g_06099584->unk14), then either seeds the camera on
 * the first frame (unk68 == 1) or eases the tracked position toward the source and
 * rebuilds the look-at angles via lookat_angles_xz (func_0221055c).
 *
 *   mode 0            : nothing to do.
 *   mode 1            : source = (*g_06099584->unk30)->unk8                  kind = 1
 *   mode 2            : source = (*g_06099584->unk34)->unk8                  kind = 1
 *   mode 3            : src  = (*g_06099584->unk30)->unk8,
 *                       src2 = (*g_06099584->unk34)->unk8                    kind = 2
 *   default (mode>3)  : source = g_06099584 itself, kind = mode (inert in the
 *                       step path; only the first-frame seed copy runs).
 *
 * The camera carries two position triples at obj+0x90 (X=90,Y=94,Z=98) and
 * obj+0x9C (X=9C,Y=A0,Z=A4); the Y component passed to the look-at is the fixed
 * height 0x14000.  kind 1 tracks one source (X=90,Z=98); kind 2 tracks two sources
 * and feeds their midpoint ((90+9C)/2, (98+A4)/2) to the look-at.
 *
 * ABI / branches: sdiv32 (0x0224a06c, r1/r0) and func_0221055c (0x0221055c,
 * lookat_angles_xz, 8-arg) resolved via braf_resolve; all other bsrf in this
 * function are phantom literal-pool longs (0x0003xxxx == `bsrf r0`), not branches.
 * The two "divide by 2" averages use the SH-2 addc/shar round-toward-zero idiom,
 * which is exactly C's signed /2.  fidelity: HIGH - verified against asm. */
extern s32  sdiv32(s32 dividend, s32 divisor);                                 /* 0x0224a06c */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *a6, s16 *a7);                                    /* lookat_angles_xz */
extern u8   g_actor_slot_registry[];                                                      /* camera control block */

void camera_ease_source_pose(void *arg0, void *arg1, void *arg2) {
    s32 kind;
    void *src  = (void *)g_actor_slot_registry;
    void *src2 = (void *)0;
    s32 mode   = M32(g_actor_slot_registry, 0x10) + M32(g_actor_slot_registry, 0x14);

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
        M32(arg0, 0x08) = M32(arg1, 0x00);
        M32(arg0, 0x0C) = M32(arg1, 0x04);
        M32(arg0, 0x10) = M32(arg1, 0x08);
        M16(arg0, 0x24) = M16(arg2, 0x00);
        M16(arg0, 0x26) = M16(arg2, 0x02);
        M16(arg0, 0x28) = M16(arg2, 0x04);
        M32(arg0, 0x68) = 0;
        if (kind == 1) {
            M32(arg0, 0x90) = M32(src, 0x08);
            M32(arg0, 0x94) = M32(src, 0x0C);
            M32(arg0, 0x98) = M32(src, 0x10);
        }
        if (kind != 2) {
            return;
        }
        M32(arg0, 0x90) = M32(src, 0x08);
        M32(arg0, 0x94) = M32(src, 0x0C);
        M32(arg0, 0x98) = M32(src, 0x10);
        M32(arg0, 0x9C) = M32(src2, 0x08);
        M32(arg0, 0xA0) = M32(src2, 0x0C);
        M32(arg0, 0xA4) = M32(src2, 0x10);
        return;
    }

    if (kind == 1) {
        M32(arg0, 0x90) += sdiv32(M32(src, 0x08) - M32(arg0, 0x90), 4);
        M32(arg0, 0x98) += sdiv32(M32(src, 0x10) - M32(arg0, 0x98), 4);
        camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                      M32(arg0, 0x90), 0x14000, M32(arg0, 0x98),
                      (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
        return;
    }

    if (kind == 2) {
        M32(arg0, 0x90) += sdiv32(M32(src,  0x08) - M32(arg0, 0x90), 4);
        M32(arg0, 0x98) += sdiv32(M32(src,  0x10) - M32(arg0, 0x98), 4);
        M32(arg0, 0x9C) += sdiv32(M32(src2, 0x08) - M32(arg0, 0x9C), 4);
        M32(arg0, 0xA4) += sdiv32(M32(src2, 0x10) - M32(arg0, 0xA4), 4);
        camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                      (M32(arg0, 0x90) + M32(arg0, 0x9C)) / 2, 0x14000,
                      (M32(arg0, 0x98) + M32(arg0, 0xA4)) / 2,
                      (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
        return;
    }
}

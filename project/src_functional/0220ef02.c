/* Round 4: 060130a4 falls through to 060130a6..130bc velocity integration after look-at; remove the premature steady-state return. Evidence: work/decomp4/fix3.asm. */
/* camera_integrate_source_follow [0x0220ef02]: Advances position by velocity and derives the eased source-relative look-at angles. */
#include "dh.h"
/* 0x0220ef02 - camera-mode service (sibling of 0x0220ea06 / 0x0220f454). Same mode
 * dispatch off the control block g_06099584 (mode = unk10 + unk14), but this variant
 * carries a world position (unk8/C/10) + velocity (unk14/18/1C) and feeds the eased
 * camera position through the transform func_0221055c (an 8-arg lookat-shaped call,
 * x,y,z,bx,by,bz,&a6,&a7; the Y component is offset by the fixed height 0x15000).
 *
 *   mode 0            : nothing.
 *   mode 1            : source = (*g_06099584->unk30)->unk8                  kind = 1
 *   mode 2            : source = (*g_06099584->unk34)->unk8                  kind = 1
 *   mode 3            : src  = (*g_06099584->unk30)->unk8,
 *                       src2 = (*g_06099584->unk34)->unk8                    kind = 2
 *   default (mode>3)  : kind = mode (inert; no valid source).
 *
 * First frame (unk68 == 1): seed world pos from arg1, velocity from arg2, seed the
 * camera triple(s) from the source(s), and run the transform once from the primary
 * source. Subsequent frames: ease the camera triple toward the source (delta/8 via
 * sdiv32) and re-run the transform; when kind == 0 just integrate world pos by the
 * velocity.
 *
 * ABI / branches: sdiv32 (0x0224a06c, r1/r0) and func_0221055c (0x0221055c, reached
 * via mova+0x15D4 jsr @r10 = 0x20ef88+0x15D4 = 0x21055c lookat_angles_xz) resolved by hand;
 * the six "TABLE/COMPUTED via bsrf r0"
 * braf_resolve reports at 0x20ef54..0x20ef68 are PHANTOM - they are the neighbouring
 * func_0220ebde literal pool (0x0003xxxx sdiv deltas) physically embedded in this
 * function's address range, not real branches. The kind==0 step (position integrate)
 * is structurally present but unreachable for mode in {0..3} (mode 0 returns early).
 * fidelity: HIGH - verified against asm; func_0221055c arg shape per call site. */
extern s32  sdiv32(s32 dividend, s32 divisor);                                 /* 0x0224a06c */
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                          s16 *a6, s16 *a7);                                    /* lookat-shaped transform */
extern u8   g_actor_slot_registry[];                                                      /* camera control block */

void camera_integrate_source_follow(void *arg0, void *arg1, void *arg2) {
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
        M32(arg0, 0x08) = M32(arg1, 0x00);
        M32(arg0, 0x0C) = M32(arg1, 0x04);
        M32(arg0, 0x10) = M32(arg1, 0x08);
        M32(arg0, 0x14) = M32(arg2, 0x00);
        M32(arg0, 0x18) = M32(arg2, 0x04);
        M32(arg0, 0x1C) = M32(arg2, 0x08);
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

    if (kind != 0) {                            /* ease toward source + transform */
        M32(arg0, 0x90) += sdiv32(M32(src, 0x08) - M32(arg0, 0x90), 8);
        M32(arg0, 0x94) += sdiv32(M32(src, 0x0C) - M32(arg0, 0x94), 8);
        M32(arg0, 0x98) += sdiv32(M32(src, 0x10) - M32(arg0, 0x98), 8);
        camera_compute_lookat_angles(M32(arg0, 0x08), M32(arg0, 0x0C), M32(arg0, 0x10),
                      M32(arg0, 0x90), M32(arg0, 0x94) + 0x15000, M32(arg0, 0x98),
                      (s16 *)((u8 *)arg0 + 0x24), (s16 *)((u8 *)arg0 + 0x26));
    }

    M32(arg0, 0x08) += M32(arg0, 0x14);         /* kind == 0: integrate world pos */
    M32(arg0, 0x0C) += M32(arg0, 0x18);
    M32(arg0, 0x10) += M32(arg0, 0x1C);
}

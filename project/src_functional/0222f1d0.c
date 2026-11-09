/* collision_test_swept_hitbox [0x0222f1d0]: Tests attacker radius against target motion segment; records hit and knockback angles. */
#include "dh.h"
/* 0x0222f1d0  hit_check_swept
 *
 * Swept collision test between an attacker hitbox (arg0, radius unkA8) and a
 * target actor (arg1, radius unk64); returns 1 and registers the hit on success,
 * 0 otherwise. Returns 0 immediately if the attacker radius unkA8 is zero.
 *
 * It measures the squared closest-approach distance between the attacker's point
 * and the target's motion segment (prev->cur position):
 *   motion = tgt.prev - tgt.cur           (unk4C/50/54 - unk8/C/10)
 *   rel    = atk.cur  - tgt.cur
 *   proj   = dot(rel, motion)
 *   if proj <= 0            : dist2 = |rel|^2                      (before segment)
 *   else if proj >= |motion|^2 : dist2 = |atk.cur - tgt.prev|^2   (after segment)
 *   else                    : t = proj / |motion|^2;
 *                             dist2 = |rel - t*motion|^2          (along segment)
 * A hit is when dist2 < (unk64 + unkA8)^2.
 *
 * On a hit it records the target data into the attacker (unk48=1, unk4A=tgt.unk34,
 * unkB8=tgt.unk5F, unkB4=tgt.unk38, unkBA=tgt.unk62, unk49=0, unk64=tgt.unk60,
 * unk74=|motion|^2), computes the knockback angles into atk.unk60/62 (special
 * fixed-angle 0x2000 path when tgt.unk38==0x40000, otherwise the full lookat),
 * and marks the target (unk5D=atk.unk34, unk5E=0, unk68 |= g_0604ebe4[unk5D]).
 *
 * Previously deferred: m2c failed (branch-into-branch) plus unresolved helpers.
 * Helpers resolved by braf_resolve.py / mova offsets:
 *   0x2132c4 vec3_dot_fixed   0x2131c8 fixmul_16_16   0x213234 sh2_hw_divide
 *   0x2107dc angle-from-2d (writes -angle)  0x21055c lookat_angles_xz
 *
 * fidelity: HIGH - the point-to-segment math, the three distance branches, the
 * hit-registration field copies and both knockback-angle paths read from ROM asm;
 * helper targets confirmed by braf_resolve.py.
 */
extern s32 g_actor_slot_masks[];
extern s32 vector_dot_fixed16(s32 *a, s32 *b);  /* vec3_dot_fixed (16.16) */
extern s32 fixed_mul16(s32 a, s32 b);    /* fixmul_16_16 */
extern s32 sh2_hw_divide(s32 num, s32 den);/* sh2_hw_divide */
extern void store_angle_between(s32 x, s32 y, s32 bx, s32 by, s16 *ang);
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                         s16 *ang0, s16 *ang1);

s32 collision_test_swept_hitbox(void *atk, void *tgt) {
    s32 motion[3];
    s32 rel[3];
    s32 closest[3];
    s32 lensq_motion, proj, dist2, radius;

    if (M32(atk, 0xA8) == 0)
        return 0;

    motion[0] = M32(tgt, 0x4C) - M32(tgt, 0x08);
    motion[1] = M32(tgt, 0x50) - M32(tgt, 0x0C);
    motion[2] = M32(tgt, 0x54) - M32(tgt, 0x10);
    lensq_motion = vector_dot_fixed16(motion, motion);

    rel[0] = M32(atk, 0x08) - M32(tgt, 0x08);
    rel[1] = M32(atk, 0x0C) - M32(tgt, 0x0C);
    rel[2] = M32(atk, 0x10) - M32(tgt, 0x10);
    proj = vector_dot_fixed16(rel, motion);

    if (proj > 0) {
        if (proj >= lensq_motion) {
            closest[0] = M32(atk, 0x08) - M32(tgt, 0x4C);
            closest[1] = M32(atk, 0x0C) - M32(tgt, 0x50);
            closest[2] = M32(atk, 0x10) - M32(tgt, 0x54);
        } else {
            s32 t = sh2_hw_divide(proj, lensq_motion);
            closest[0] = rel[0] - fixed_mul16(t, motion[0]);
            closest[1] = rel[1] - fixed_mul16(t, motion[1]);
            closest[2] = rel[2] - fixed_mul16(t, motion[2]);
        }
        dist2 = vector_dot_fixed16(closest, closest);
    } else {
        dist2 = vector_dot_fixed16(rel, rel);
    }

    radius = M32(tgt, 0x64) + M32(atk, 0xA8);
    if (dist2 >= fixed_mul16(radius, radius))
        return 0;

    M8(atk, 0x48)  = 1;
    M8(atk, 0x4A)  = M8(tgt, 0x34);
    M16(atk, 0xB8) = (u8)M8(tgt, 0x5F);
    M32(atk, 0xB4) = M32(tgt, 0x38);
    M16(atk, 0xBA) = M16(tgt, 0x62);
    M8(atk, 0x49)  = 0;
    M16(atk, 0x64) = M16(tgt, 0x60);
    M32(atk, 0x74) = lensq_motion;

    if (M32(tgt, 0x38) == 0x40000) {
        store_angle_between(M32(tgt, 0x4C), M32(tgt, 0x54), M32(atk, 0x08),
                     M32(atk, 0x10), (s16 *)((u8 *)atk + 0x62));
        M16(atk, 0x60) = 0x2000;
    } else {
        camera_compute_lookat_angles(M32(tgt, 0x4C), M32(tgt, 0x50), M32(tgt, 0x54),
                     M32(tgt, 0x08), M32(tgt, 0x0C), M32(tgt, 0x10),
                     (s16 *)((u8 *)atk + 0x60), (s16 *)((u8 *)atk + 0x62));
    }

    M8(tgt, 0x5D) = M8(atk, 0x34);
    M8(tgt, 0x5E) = 0;
    M32(tgt, 0x68) |= g_actor_slot_masks[M8(tgt, 0x5D)];
    return 1;
}

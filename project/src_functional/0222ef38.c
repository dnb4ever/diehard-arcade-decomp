/* collision_test_pose_hitboxes [0x0222ef38]: Tests up to ten pose hitboxes against swept target motion; records first hit. */
#include "dh.h"
/* 0x0222ef38  actor_multi_hitbox_sweep_test
 *
 * ABI: s32 func_0222ef38(void *atk, void *tgt)   (r4=atk, r5=tgt)
 *
 * Scans up to 10 of the attacker's active sub-hitboxes against the target's
 * swept motion segment and, on the first hit (highest index first), registers
 * the hit into the attacker and the target and returns 1; returns 0 if none hit.
 *
 * It is a 10-iteration specialisation of the sibling swept test func_0222f1d0
 * (hit_check_swept): the per-target point-to-segment distance math is identical,
 * but here the "point" is drawn from an array of candidate hitboxes belonging to
 * the attacker's pose record rather than a single attacker position.
 *
 * Setup (once):
 *   seg   = tgt.prev - tgt.cur         (tgt.4C/50/54 - tgt.08/0C/10)   [motion]
 *   lensq = dot(seg, seg)                                               [|seg|^2 -> r8]
 *   radp  = &g_0607C424[atk.5E*0x40 + 0x24]   (candidate radius, s32, -4 per iter)
 *   rec   = &g_06097D58[atk.34*0x100 + 0x90]  (candidate xyz, 16B rec, -16 per iter)
 *
 * Per candidate i = 9..0 (rec/radp walk downward):
 *   cand_r = *radp;  if (cand_r == 0) skip (empty slot)
 *   broad-phase: if (!bounds_overlap_margin(g_06097444, g_06097438, rec, cand_r)) skip
 *   rel  = rec.xyz - tgt.cur
 *   proj = dot(rel, seg)
 *     proj <= 0           : dist2 = |rel|^2                     (before segment)
 *     0 < proj < lensq    : t = proj/lensq;
 *                           dist2 = |rel - t*seg|^2             (along segment)
 *     proj >= lensq       : dist2 = |rec.xyz - tgt.prev|^2      (after segment)
 *   rad = cand_r + tgt.64;  hit when dist2 < (rad)^2 (fixmul)
 *
 * On hit: record into attacker (atk.48=1, atk.4A=tgt.34, atk.B8=tgt.5F,
 *   atk.B4=tgt.38, atk.BA=tgt.62, atk.49=i, atk.64=tgt.60, atk.74=lensq,
 *   atk.138=tgt.4C), compute knockback angle(s) (fixed-angle path when
 *   tgt.38==0x40000 via store_angle_between, else full lookat via func_0221055c),
 *   gated sfx (((g_06053F04[tgt.5C]) << 8) | 0xA00000FF) when enabled,
 *   mark target (tgt.5D=atk.34, tgt.5E=i, tgt.68 |= g_0604EBE4[tgt.5D]); return 1.
 *
 * m2c failed here ("Cannot find branch target .L22f1b2": the mid-body literal
 * pools at 0x22f01a and 0x22f182 were mis-decoded as code, creating a phantom
 * branch-into-branch). Control flow was hand-traced from fn.py's annotated
 * targets; the two PC-relative helpers were resolved from mova+add anchors
 * (r11 -> 0x022132c4 vec3_dot_fixed, r10 -> 0x022131c8 fixmul_16_16) and the
 * five bsrf calls from braf_resolve.py (0x21330c, 0x213234, 0x2107dc, 0x21055c,
 * 0x215f6c). The two hit-response calls carry dropped stack args (1 and 4
 * respectively), declared with full arg lists so marshalling is preserved.
 *
 * fidelity: HIGH - every field access, the three distance branches, both
 * knockback paths, the gated sfx and the loop pointer/index decrements read
 * from ROM asm; helper targets confirmed by braf_resolve.py / mova offsets.
 */
extern s32 g_actor_slot_masks[];

extern s32  vector_dot_fixed16(s32 *a, s32 *b);   /* vec3_dot_fixed (16.16)        */
extern s32  fixed_mul16(s32 a, s32 b);     /* fixmul_16_16                  */
extern s32  sh2_hw_divide(s32 num, s32 den); /* sh2_hw_divide                 */
extern s32  bounds_overlap_margin(s32 *a, s32 *b, s32 *c, s32 margin); /* bounds_overlap_margin */
extern void store_angle_between(s32 x, s32 y, s32 bx, s32 by, s16 *ang);
extern void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz,
                         s16 *ang0, s16 *ang1);
extern void sound_queue_push(s32 cmd);          /* sound_queue_push              */

s32 collision_test_pose_hitboxes(void *atk, void *tgt) {
    s32 seg[3], rel[3], closest[3];
    s32 lensq, proj, dist2, rad;
    s32 i;
    s32 *radp;
    u8  *rec;

    seg[0] = M32(tgt, 0x4C) - M32(tgt, 0x08);
    seg[1] = M32(tgt, 0x50) - M32(tgt, 0x0C);
    seg[2] = M32(tgt, 0x54) - M32(tgt, 0x10);
    lensq = vector_dot_fixed16(seg, seg);

    radp = (s32 *)(uintptr_t)(0x0607C424 + (((s32)M16(atk, 0x5E)) << 6) + 0x24);
    rec  = (u8  *)(uintptr_t)(0x06097D58 + (((s32)(s8)M8(atk, 0x34)) << 8) + 0x90);

    for (i = 9; i >= 0; i--, radp--, rec -= 16) {
        s32 cand_r = *radp;
        if (cand_r == 0)
            continue;
        if (bounds_overlap_margin((s32 *)(uintptr_t)0x06097444,
                          (s32 *)(uintptr_t)0x06097438,
                          (s32 *)rec, cand_r) == 0)
            continue;

        rel[0] = *(s32 *)(rec + 0) - M32(tgt, 0x08);
        rel[1] = *(s32 *)(rec + 4) - M32(tgt, 0x0C);
        rel[2] = *(s32 *)(rec + 8) - M32(tgt, 0x10);
        proj = vector_dot_fixed16(rel, seg);

        if (proj <= 0) {
            dist2 = vector_dot_fixed16(rel, rel);
        } else {
            if (proj < lensq) {
                s32 t = sh2_hw_divide(proj, lensq);
                closest[0] = rel[0] - fixed_mul16(t, seg[0]);
                closest[1] = rel[1] - fixed_mul16(t, seg[1]);
                closest[2] = rel[2] - fixed_mul16(t, seg[2]);
            } else {
                closest[0] = *(s32 *)(rec + 0) - M32(tgt, 0x4C);
                closest[1] = *(s32 *)(rec + 4) - M32(tgt, 0x50);
                closest[2] = *(s32 *)(rec + 8) - M32(tgt, 0x54);
            }
            dist2 = vector_dot_fixed16(closest, closest);
        }

        rad = cand_r + M32(tgt, 0x64);
        if (dist2 >= fixed_mul16(rad, rad))
            continue;

        /* hit registered on the attacker */
        M8(atk, 0x48)  = 1;
        M8(atk, 0x4A)  = M8(tgt, 0x34);
        M16(atk, 0xB8) = (u8)M8(tgt, 0x5F);
        M32(atk, 0xB4) = M32(tgt, 0x38);
        M16(atk, 0xBA) = M16(tgt, 0x62);
        M8(atk, 0x49)  = (s8)i;
        M16(atk, 0x64) = M16(tgt, 0x60);
        M32(atk, 0x74) = lensq;

        if (M32(tgt, 0x38) == 0x40000) {
            store_angle_between(M32(tgt, 0x4C), M32(tgt, 0x54), M32(atk, 0x08),
                         M32(atk, 0x10), (s16 *)((u8 *)atk + 0x62));
        } else {
            camera_compute_lookat_angles(M32(tgt, 0x4C), M32(tgt, 0x50), M32(tgt, 0x54),
                         M32(tgt, 0x08), M32(tgt, 0x0C), M32(tgt, 0x10),
                         (s16 *)((u8 *)atk + 0x60), (s16 *)((u8 *)atk + 0x62));
        }

        {
            s8 sfx = M8(tgt, 0x5C);
            if (sfx != 0 && *(s32 *)(uintptr_t)DH_SOUND_EFFECTS_ENABLED_ADDR != 0) {
                s16 id = ((s16 *)(uintptr_t)0x06053F04)[sfx];
                sound_queue_push((s32)(((u32)((s32)id << 8)) | DH_SOUND_CUE_BASE_U));
            }
        }

        M32(atk, 0x138) = M32(tgt, 0x4C);
        M8(tgt, 0x5D) = M8(atk, 0x34);
        M8(tgt, 0x5E) = (s8)i;
        M32(tgt, 0x68) |= g_actor_slot_masks[M8(tgt, 0x5D)];
        return 1;
    }
    return 0;
}

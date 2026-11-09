/* camera_compute_xy_lookat [0x02210622]: Computes heading and elevation with XY-plane axis pairing. */
#include "dh.h"
/* 0x02210622 (look-at angles from two world points, XY-plane variant) - sibling of
 * func_0221055c with a different axis pairing:
 *   p = a4 - a1, q = a5 - a2, r = a6 - a3
 *   *a7 = -atan2_fixed(p, q)                    (planar heading, negated)
 *   dist = sqrt(fixmul(q,q) + fixmul(p,p))     (note: q first, matching call order)
 *   *a8 =  atan2_fixed(dist, r)                 (elevation)
 * ABI: r4-r7 = a1-a4, stack = a5-a8 (a7/a8 are s16* outputs).
 * fidelity: HIGH - deltas and call order verified vs asm: r14 = @(36,r15)-@(0,r15),
 * r13 = r7 - @(12,r15), fixmul(r14,r14) precedes fixmul(r13,r13), sum r8+r0 -> sqrt. */
extern s32 atan2_fixed(s32 a, s32 b);
extern s32 fixed_mul16(s32 a, s32 b);
extern u32 fixed_sqrt16(u32 x);

void camera_compute_xy_lookat(s32 a1, s32 a2, s32 a3, s32 a4, s32 a5, s32 a6, s16 *a7, s16 *a8) {
    s32 p = a4 - a1;
    s32 q = a5 - a2;
    s32 r = a6 - a3;
    s32 fq;
    s32 fp;
    s32 dist;

    *a7 = (s16)(-atan2_fixed(p, q));
    fq = fixed_mul16(q, q);
    fp = fixed_mul16(p, p);
    dist = (s32)fixed_sqrt16((u32)(fq + fp));
    *a8 = (s16)atan2_fixed(dist, r);
}

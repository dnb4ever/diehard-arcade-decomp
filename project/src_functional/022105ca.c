/* camera_angles_from_delta [0x022105ca]: Computes XZ heading and elevation from the supplied coordinate delta triple. */
#include "dh.h"
/* 0x022105ca (look-at angles from a precomputed delta, XZ-plane variant) - like
 * func_0221055c but the caller passes the delta components directly as (dx,dy,dz).
 *   *a4 = -angle(dz, dx)                        (heading in the XZ plane, negated)
 *   dist = sqrt(fixmul(dx,dx) + fixmul(dz,dz))  (horizontal distance)
 *   *a3 =  angle(dist, dy)                       (elevation)
 * angle = atan2_fixed, fixmul = func_022131c8, sqrt = func_02213164 (all out of band).
 * fidelity: HIGH - deltas 32-bit (16.16), angle outputs 16-bit; computed sibling calls
 * resolved via braf_resolve. Verified vs asm. */
extern s32 atan2_fixed(s32 a, s32 b);
extern s32 fixed_mul16(s32 a, s32 b);
extern s32 fixed_sqrt16(s32 x);

void camera_angles_from_delta(s32 dx, s32 dy, s32 dz, s16 *a3, s16 *a4) {
    s32 dist;

    *a4 = (s16)(-atan2_fixed(dz, dx));
    dist = fixed_sqrt16(fixed_mul16(dx, dx) + fixed_mul16(dz, dz));
    *a3 = (s16)atan2_fixed(dist, dy);
}

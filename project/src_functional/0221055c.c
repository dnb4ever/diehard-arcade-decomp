/* camera_compute_lookat_angles [0x0221055c]: Computes XZ heading and elevation from two 3D positions using fixed multiply, sqrt and atan2. */
#include "dh.h"
/* 0x0221055c (look-at angles from two world points, XZ-plane variant) - given a point
 * (x,y,z) and a base/target point (bx,by,bz), computes the delta (dx,dy,dz) and writes two
 * 16-bit angles: the horizontal (XZ) heading and the elevation.
 *   dx=bx-x, dy=by-y, dz=bz-z
 *   *a7 = -angle(dz, dx)                       (heading in the XZ plane, negated)
 *   dist = sqrt(fixmul(dx,dx) + fixmul(dz,dz)) (horizontal distance)
 *   *a6 =  angle(dist, dy)                      (elevation)
 * angle() = atan2_fixed (planar atan2-style helper, out of band), fixmul = func_022131c8
 * (16.16 multiply), sqrt = func_02213164 (16.16 sqrt). Called by func_0220d262.
 * fidelity: HIGH - coords 32-bit (16.16), angle outputs 16-bit; computed sibling calls
 * resolved via braf_resolve; m2c could not lower the bsrf chain. Verified vs asm. */
extern s32 atan2_fixed(s32 a, s32 b);
extern s32 fixed_mul16(s32 a, s32 b);
extern s32 fixed_sqrt16(s32 x);

void camera_compute_lookat_angles(s32 x, s32 y, s32 z, s32 bx, s32 by, s32 bz, s16 *a6, s16 *a7) {
    s32 dx = bx - x;
    s32 dy = by - y;
    s32 dz = bz - z;
    s32 dist;

    *a7 = (s16)(-atan2_fixed(dz, dx));
    dist = fixed_sqrt16(fixed_mul16(dx, dx) + fixed_mul16(dz, dz));
    *a6 = (s16)atan2_fixed(dist, dy);
}

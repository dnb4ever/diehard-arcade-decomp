/* distance_squared_xz [0x022131d4]: Signed 16.16 products of X and Z differences. */
#include "dh.h"
/* 0x022131d4 (dist2_xz_fixed) - squared distance between two points on the X/Z plane in 16.16
 * fixed point: dx = a->unk0 - b->unk0, dz = a->unk8 - b->unk8, return fixmul(dx,dx) + fixmul(dz,dz),
 * where fixmul(u,v) = (s32)(((s64)u * v) >> 16) (the dmuls.l + xtrct 16.16 product idiom).
 * fidelity: HIGH - coords long; dmuls.l is signed 32x32->64, xtrct r(mach),r(macl) yields the middle
 * 32 bits = product >> 16. */
s32 distance_squared_xz(void *a, void *b) {
    s32 dx = M32(a, 0x0) - M32(b, 0x0);
    s32 dz = M32(a, 0x8) - M32(b, 0x8);
    return (s32)(((s64)dx * dx) >> 16) + (s32)(((s64)dz * dz) >> 16);
}

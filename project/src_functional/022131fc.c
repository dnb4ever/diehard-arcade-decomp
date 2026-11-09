/* distance_squared_xyz [0x022131fc]: Signed 16.16 products of all three coordinate differences. */
#include "dh.h"
/* 0x022131fc (dist2_xyz_fixed) - squared distance between two 3-vectors in 16.16 fixed point:
 * dx = a->unk0 - b->unk0, dy = a->unk4 - b->unk4, dz = a->unk8 - b->unk8,
 * return fixmul(dx,dx) + fixmul(dy,dy) + fixmul(dz,dz), fixmul(u,v) = (s32)(((s64)u*v)>>16).
 * 3-axis sibling of dist2_xz_fixed (0x022131d4).
 * fidelity: HIGH - all coords long; dmuls.l/xtrct 16.16 product idiom. */
s32 distance_squared_xyz(void *a, void *b) {
    s32 dx = M32(a, 0x0) - M32(b, 0x0);
    s32 dy = M32(a, 0x4) - M32(b, 0x4);
    s32 dz = M32(a, 0x8) - M32(b, 0x8);
    return (s32)(((s64)dx * dx) >> 16)
         + (s32)(((s64)dy * dy) >> 16)
         + (s32)(((s64)dz * dz) >> 16);
}

/* aabb_overlap_box [0x02244874]: Existing map corroborated by source: AABB vs collision box g_060B6D9C[arg0->unk34] (0x48 stride): x margin 0x30000/box+0x18, z margin 0x10000/box+0x20; returns arg1 if overlap else NULL. (fixed draft signedness: -0x30000 not +0xFFFD0000) */
#include "dh.h"
/* 0x02244874 (aabb_overlap_box) - test an AABB against the collision box selected by
 * arg0->unk34 in g_060B6D9C (0x48-byte elements). Center arg2 (unk0=x, unk8=z), object
 * extents arg1 (unk8=x, unk10=z) with fixed margins 0x30000 (x) / 0x10000 (z) and box
 * half-extents box->unk18 (x) / box->unk20 (z). Returns arg1 when the two ranges overlap
 * on both axes, else NULL.
 * fidelity: HIGH - all signed compares (ROM uses cmp/gt + cmp/ge; the -0x30000/-0x10000
 * subtractions keep the arithmetic signed, unlike a +0xFFFD0000 unsigned promotion). */
extern u8 g_actor_movement_records[];   /* collision-box array, 0x48-byte elements */

void *aabb_overlap_box(void *arg0, void *arg1, void *arg2) {
    u8 *box = &g_actor_movement_records[(s16)(M8(arg0, 0x34) * 0x48)];
    s32 ax = M32(arg1, 0x08), cx = M32(arg2, 0x00), hx = M32(box, 0x18);

    if ((ax + 0x30000) > (cx - hx) && (ax - 0x30000) < (cx + hx)) {
        s32 az = M32(arg1, 0x10), cz = M32(arg2, 0x08), hz = M32(box, 0x20);
        if ((az + 0x10000) > (cz - hz) && (az - 0x10000) < (cz + hz))
            return arg1;
    }
    return (void *)0;
}

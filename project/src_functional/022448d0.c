/* aabb_overlap_box_b [0x022448d0]: Existing map corroborated by source: variant of 0x244874: object x=arg1->unk0, z=arg1->unk8; returns 1/0 */
#include "dh.h"
/* 0x022448d0 (aabb_overlap_box_b) - variant of aabb_overlap_box (0x02244874) that takes
 * the object x from arg1->unk0 (not unk8), the z from arg1->unk8 (not unk10), and returns
 * 1/0 instead of the pointer. Box selected by arg0->unk34 in g_060B6D9C; same margins and
 * box half-extents (unk18 x, unk20 z).
 * fidelity: HIGH - signed compares; -0x30000/-0x10000 kept signed. */
extern u8 g_actor_movement_records[];   /* collision-box array, 0x48-byte elements */

s32 aabb_overlap_box_b(void *arg0, void *arg1, void *arg2) {
    u8 *box = &g_actor_movement_records[(s16)(M8(arg0, 0x34) * 0x48)];
    s32 ax = M32(arg1, 0x00), cx = M32(arg2, 0x00), hx = M32(box, 0x18);

    if ((ax + 0x30000) > (cx - hx) && (ax - 0x30000) < (cx + hx)) {
        s32 az = M32(arg1, 0x08), cz = M32(arg2, 0x08), hz = M32(box, 0x20);
        if ((az + 0x10000) > (cz - hz) && (az - 0x10000) < (cz + hz))
            return 1;
    }
    return 0;
}

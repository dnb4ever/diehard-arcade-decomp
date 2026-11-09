/* actor_test_target_rectangle [0x0224492c]: Tests actor XZ position against the target-relative rectangle and supplied half-extents. */
#include "dh.h"
/* 0x0224492c (point_in_box) - test whether a point lies inside arg0''s box. The box center
 * is (arg1->unk8 + arg0->unk130, arg1->unk10 + arg0->unk134); the point is (arg0->unk8,
 * arg0->unk10); half-extents are arg2 (x) and arg3 (z). Returns 1 when the point is within
 * [center-ext, center+ext] on both axes, else 0 (early-out per axis).
 * fidelity: HIGH - signed cmp/ge per edge. */

s32 actor_test_target_rectangle(void *arg0, void *arg1, s32 arg2, s32 arg3) {
    s32 cx = M32(arg1, 0x08) + M32(arg0, 0x130);
    s32 cz = M32(arg1, 0x10) + M32(arg0, 0x134);
    s32 px = M32(arg0, 0x08);
    s32 pz;

    if (px < (cx - arg2))
        return 0;
    if ((arg2 + cx) < px)
        return 0;
    pz = M32(arg0, 0x10);
    if (pz < (cz - arg3))
        return 0;
    if ((arg3 + cz) < pz)
        return 0;
    return 1;
}

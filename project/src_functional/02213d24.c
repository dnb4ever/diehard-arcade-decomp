/* quad_build_projected_corners [0x02213d24]: Copies four indexed screen positions and depths into a quad transform record. */
#include "dh_band21.h"
/* 0x02213d24 - assemble a 4-corner sprite/quad transform record. idx is a 4-byte
 * index array; each byte selects an entry (stride 0x10) in the source table base.
 * Per corner i (ROM order): load src[0] and src[4]; store s16 src[0] at out+k,
 * store s32 src[8] at out+16+4i, then store s16 -src[4] at out+k+2, where
 * k = 4i. fidelity: faithful - exact load/store ordering per corner
 *   (mov.w @(0,r5) ; mov.l @(16,r5) ; neg ; mov.w @(2,r5)), stride 0x10 via
 *   extu.b + shll2 x4, s16 truncation of src[0] and of the negated src[4]. */
void quad_build_projected_corners(u32 idx, u32 out, u32 base)
{
    u32 p;
    s32 v0;
    s32 v4;
    s32 v8;

    p = base + MEM8(idx + 0) * 0x10u;
    v0 = MEM32(p + 0);
    v4 = MEM32(p + 4);
    MEM16(out + 0) = (s16)v0;
    v8 = MEM32(p + 8);
    MEM32(out + 16) = v8;
    MEM16(out + 2) = (s16)(-v4);

    p = base + MEM8(idx + 1) * 0x10u;
    v0 = MEM32(p + 0);
    v4 = MEM32(p + 4);
    MEM16(out + 4) = (s16)v0;
    v8 = MEM32(p + 8);
    MEM32(out + 20) = v8;
    MEM16(out + 6) = (s16)(-v4);

    p = base + MEM8(idx + 2) * 0x10u;
    v0 = MEM32(p + 0);
    v4 = MEM32(p + 4);
    MEM16(out + 8) = (s16)v0;
    v8 = MEM32(p + 8);
    MEM32(out + 24) = v8;
    MEM16(out + 10) = (s16)(-v4);

    p = base + MEM8(idx + 3) * 0x10u;
    v0 = MEM32(p + 0);
    v4 = MEM32(p + 4);
    MEM16(out + 12) = (s16)v0;
    v8 = MEM32(p + 8);
    MEM32(out + 28) = v8;
    MEM16(out + 14) = (s16)(-v4);
}

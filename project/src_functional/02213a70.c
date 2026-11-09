/* 0x02213a70  project_quad_corners: build the 4 corners (+/-bx, +/-by) of a quad
 * from two scaled half-extents and project each through the active 3x4 matrix
 * @g_060da040, writing 4 packed screen positions to g_06098b1c[0..3].
 * args (half_w, half_h, scale_x, scale_y):
 *   by = ((s64)(half_h << 8) * scale_y) >> 16         (y extent)
 *   bx = ((s64)((half_w << 8) >> 1) * scale_x) >> 16  (x extent; shar = >>1)
 *   corners = { (-bx,by), (bx,by), (bx,-by), (-bx,-by) }   (stacked -a/a/b layout)
 * per corner (px,py):  depth = ((M[0]*px + M[1]*py) >> 16) + M[3]   (no clamp here)
 *   recip = (0x100<<32)/depth                          (DIVU 64/32)
 *   row1 = ((M[4]*px + M[5]*py) >> 16) + M[7]
 *   row2 = ((M[8]*px + M[9]*py) >> 16) + M[11]
 *   sx = ((row2*recip)>>16) + 0x8000;  sy = ((row1*recip)>>16) + 0x8000
 *   out = (((sx>>16)<<1)&0xffff)<<16 | ((u32)(-sy)>>16 & 0xffff)
 * fidelity: HIGH - clrmac/mac.l 2-term dot per row (3rd coeff skipped via add #4),
 *   xtrct = >>16, dmuls.l project, swap.w/neg/shll packing, DVSR/DVDNTH/DVDNTL
 *   64/32 divide w/ DVDNTUL readback, dt r8 (4 corners) loop. */
#include "dh_band21.h"

void project_quad_corners(s32 half_w, s32 half_h, s32 scale_x, s32 scale_y) {
    s32 *M   = PTR(s32, 0x060da040);
    s32 *out = PTR(s32, DH_TRANSFORMED_VECTOR_ADDR);
    s32 by = (s32)(((long long)((s32)half_h << 8) * scale_y) >> 16);
    s32 bx = (s32)(((long long)(((s32)half_w << 8) >> 1) * scale_x) >> 16);
    s32 corners[4][2] = { {-bx, by}, {bx, by}, {bx, -by}, {-bx, -by} };
    int i;

    for (i = 0; i < 4; i++) {
        s32 px = corners[i][0], py = corners[i][1];
        long long acc;
        s32 depth, row1, row2, sx, sy;
        u32 recip;

        acc = (long long)M[0] * px + (long long)M[1] * py;
        depth = (s32)(acc >> 16) + M[3];

        MEM32(DIVU_DVSR)   = depth;
        MEM32(DIVU_DVDNTH) = 0x100;
        MEM32(DIVU_DVDNTL) = 0;
        recip = MEMU32(DIVU_DVDNTUL);

        acc = (long long)M[4] * px + (long long)M[5] * py;
        row1 = (s32)(acc >> 16) + M[7];
        acc = (long long)M[8] * px + (long long)M[9] * py;
        row2 = (s32)(acc >> 16) + M[11];

        sx = (s32)(((long long)row2 * (s32)recip) >> 16) + 0x8000;
        sy = (s32)(((long long)row1 * (s32)recip) >> 16) + 0x8000;
        out[i] = (s32)(((((u32)sx >> 16) << 1) << 16) | (((u32)(-sy) >> 16) & 0xffff));
    }
}

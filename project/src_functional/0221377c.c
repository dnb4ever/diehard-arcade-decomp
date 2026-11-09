/* 0x0221377c  vertex_project_dualmtx_m: like vertex_project_dualmtx (0x213570) but
 * the matrix base is passed in (arg3 = M): M0 @+0 (w<=0), M1 @+0x30 (w>0), coord
 * offset table @+0x60 (added to x,y,z when w>0). Output array g_060dfbc8 (16-byte
 * stride). Also pre-clears 8 longs at 16-byte stride @(M + 0x40000000).
 * Per vertex (src = x,y,z,w):
 *   depth = ((s64 dot M[0..2]) >> 16) + M[3],  clamped to [0x8000, 0x208000]
 *   recip = (0x100<<32) / depth              (SH-2 DIVU 64/32)
 *   y_num = ((s64 dot M[4..6]) >> 16) + M[7]      (row 1)
 *   x_num = ((s64 dot M[8..10]) >> 16) + M[11]    (row 2)
 *   sx = ((s32)((x_num*recip)>>16)) + 0x8000;  out[0] = (s16)(sx>>16) << 1
 *   sy = ((s32)((y_num*recip)>>16)) + 0x8000;  out[1] = (s16)(sy>>16)
 * fidelity: HIGH - clrmac/mac.l x3 dot per row (xtrct = >>16), 3-step w>0 coord
 *   offset loop, cmp/gt clamp, DVSR/DVDNTH/DVDNTL 64/32 divide w/ DVDNTUL readback,
 *   dmuls.l + xtrct project, swap.w/exts.w high-half, x doubled (shll), dt r8 loop. */
#include "dh_band21.h"

void vertex_project_dualmtx_m(s32 count, s32 *verts, s32 *M) {
    s32 *zp  = (s32 *)((char *)(uintptr_t)M + 0x40000000);
    s32 *out = PTR(s32, 0x060dfbc8);
    s32 *src = verts;
    int i, n;

    for (i = 0; i < 8; i++)                 /* zero 8 longs, 16-byte stride */
        zp[i * 4] = 0;

    for (n = count; n > 0; n--) {
        s32 coord[3];
        s32 *m;
        s32 w, depth, xnum, ynum, sx, sy;
        u32 recip;
        long long acc;

        coord[0] = *src++;
        coord[1] = *src++;
        coord[2] = *src++;
        w        = *src++;

        if (w > 0) {
            s32 *off = M + 0x60 / 4;
            coord[0] += off[0];
            coord[1] += off[1];
            coord[2] += off[2];
            m = M + 0x30 / 4;
        } else {
            m = M;
        }

        acc = (long long)m[0] * coord[0] + (long long)m[1] * coord[1] + (long long)m[2] * coord[2];
        depth = (s32)(acc >> 16) + m[3];
        if (depth < 0x8000)        depth = 0x8000;
        else if (depth > 0x208000) depth = 0x208000;
        out[2] = depth;

        MEM32(DIVU_DVSR)   = depth;
        MEM32(DIVU_DVDNTH) = 0x100;
        MEM32(DIVU_DVDNTL) = 0;
        recip = MEMU32(DIVU_DVDNTUL);

        acc = (long long)m[4] * coord[0] + (long long)m[5] * coord[1] + (long long)m[6] * coord[2];
        ynum = (s32)(acc >> 16) + m[7];
        acc = (long long)m[8] * coord[0] + (long long)m[9] * coord[1] + (long long)m[10] * coord[2];
        xnum = (s32)(acc >> 16) + m[11];

        sx = (s32)(((long long)xnum * (s32)recip) >> 16) + 0x8000;
        sy = (s32)(((long long)ynum * (s32)recip) >> 16) + 0x8000;
        out[0] = ((s16)(sx >> 16)) << 1;
        out[1] = (s16)(sy >> 16);

        out += 4;
    }
}

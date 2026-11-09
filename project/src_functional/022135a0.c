/* vertex_transform_project_raw_entry [0x022135a0]: Selects one of two matrices, transforms vertices and perspective-projects them into sixteen-byte screen records. */
#include "dh_band21.h"
/* 0x022135a0 (vertex_project_dualmtx) - transform and perspective-project `count`
 * 4-component vertices (x,y,z,w) from `verts`, writing 16-byte screen records to
 * 0x060debc8. The function entry is 0x022135a0; the physically-earlier block at
 * 0x02213570 is the w>0 arm, reached by the in-loop bt.s and modelled inline here.
 *
 * Matrix base M = 0x060da040 (3 rows x 4 longs). w<=0: use M0 at +0x00, no offset.
 * w>0: add the translate at M+0x60 to (x,y,z) and use M1 at +0x30. Each row result
 * = ((row[0]*x + row[1]*y + row[2]*z) >> 16) + row[3]. Row0 = depth, clamped to
 * [0x8000, 0x208000] -> out[2]. recip = (0x100<<32)/depth via the SH-2 DIVU 64/32.
 * Row2 -> X, Row1 -> Y: out = ((s16)(((num*recip)>>16 + 0x8000) >> 16)); X doubled.
 * fidelity: HIGH - MAC dot-products, dual-matrix/offset selection, depth clamp,
 *   DIVU reciprocal, 16.16 projection, X<<1, 16-byte output stride all modelled. */
void vertex_transform_project_raw_entry(s32 count, u32 verts)
{
    u32 out = 0x060debc8;

    do {
        s32 x = MEM32(verts + 0);
        s32 y = MEM32(verts + 4);
        s32 z = MEM32(verts + 8);
        s32 w = MEM32(verts + 12);
        u32 m;
        s32 depth, xnum, ynum, recip, xp, yp;
        verts += 16;

        if (w > 0) {                          /* 0x02213570 arm */
            x += MEM32(0x060da040 + 0x60 + 0);
            y += MEM32(0x060da040 + 0x60 + 4);
            z += MEM32(0x060da040 + 0x60 + 8);
            m = 0x060da040 + 0x30;            /* M1 */
        } else {
            m = 0x060da040;                   /* M0 */
        }

        /* row 0 -> depth */
        depth = (s32)(((long long)MEM32(m + 0) * (long long)x
                     + (long long)MEM32(m + 4) * (long long)y
                     + (long long)MEM32(m + 8) * (long long)z) >> 16) + MEM32(m + 12);
        m += 16;
        if (depth < 0x8000)        depth = 0x8000;
        else if (depth > 0x208000) depth = 0x208000;
        MEM32(out + 8) = depth;

        /* recip = (0x100 << 32) / depth (DIVU 64/32) */
        MEM32(DIVU_DVSR)   = depth;
        MEM32(DIVU_DVDNTH) = 0x100;
        MEM32(DIVU_DVDNTL) = 0;
        recip = MEM32(DIVU_DVDNTUL);

        /* row 1 -> Y numerator */
        ynum = (s32)(((long long)MEM32(m + 0) * (long long)x
                    + (long long)MEM32(m + 4) * (long long)y
                    + (long long)MEM32(m + 8) * (long long)z) >> 16) + MEM32(m + 12);
        m += 16;
        /* row 2 -> X numerator */
        xnum = (s32)(((long long)MEM32(m + 0) * (long long)x
                    + (long long)MEM32(m + 4) * (long long)y
                    + (long long)MEM32(m + 8) * (long long)z) >> 16) + MEM32(m + 12);

        xp = (s32)(((long long)xnum * (long long)recip) >> 16) + 0x8000;
        yp = (s32)(((long long)ynum * (long long)recip) >> 16) + 0x8000;
        MEM32(out + 0) = (s16)(xp >> 16) << 1;
        MEM32(out + 4) = (s16)(yp >> 16);

        out += 16;
    } while (--count != 0);
}

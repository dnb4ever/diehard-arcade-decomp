/* 0x02213570  vertex_project_dualmtx: transform & perspective-project `count`
 * vertices, selecting one of two 3x4 matrices (and an added coord offset) by the
 * sign of each vertex w. Matrices live at g_060da040: M0 @+0 (w<=0), M1 @+0x30
 * (w>0); the +0x30 offset table @+0x60 is added to (x,y,z) first when w>0.
 * Output array g_060debc8, 16-byte stride (out[0]=screen x, out[1]=y, out[2]=depth).
 * Per vertex (src = x,y,z,w):
 *   depth  = ((s64 dot M[0..2]) >> 16) + M[3],  clamped to [0x8000, 0x208000]
 *   recip  = (0x100<<32) / depth            (SH-2 DIVU 64/32, unsigned-positive)
 *   y_num  = ((s64 dot M[4..6]) >> 16) + M[7]       (row 1)
 *   x_num  = ((s64 dot M[8..10]) >> 16) + M[11]     (row 2)
 *   sx = ((s32)((x_num*recip)>>16)) + 0x8000;  out[0] = (s16)(sx>>16) << 1
 *   sy = ((s32)((y_num*recip)>>16)) + 0x8000;  out[1] = (s16)(sy>>16)
 * fidelity: HIGH - clrmac/mac.l x3 dot per row (xtrct = >>16), cmp/gt clamp,
 *   DVSR/DVDNTH/DVDNTL 64/32 divide w/ DVDNTUL readback, dmuls.l + xtrct project,
 *   swap.w/exts.w high-half extract, x doubled (shll) for aspect, dt r8 loop. */
#include "dh_band21.h"

void vertex_project_dualmtx(s32 count, s32 *verts) {
    s32 *out  = PTR(s32, 0x060debc8);
    s32 *Mbase = PTR(s32, 0x060da040);
    s32 *src  = verts;
    int n;

    for (n = count; n > 0; n--) {
        s32 coord[3];
        s32 *M;
        s32 w, depth, xnum, ynum, sx, sy;
        u32 recip;
        long long acc;

        coord[0] = *src++;
        coord[1] = *src++;
        coord[2] = *src++;
        w        = *src++;

        if (w > 0) {                       /* cmp/pl r5 -> bt/s 0x213570 */
            s32 *off = Mbase + 0x60 / 4;   /* g_060da0a0 */
            coord[0] += off[0];
            coord[1] += off[1];
            coord[2] += off[2];
            M = Mbase + 0x30 / 4;          /* g_060da070 */
        } else {
            M = Mbase;                     /* g_060da040 */
        }

        acc = (long long)M[0] * coord[0] + (long long)M[1] * coord[1] + (long long)M[2] * coord[2];
        depth = (s32)(acc >> 16) + M[3];
        if (depth < 0x8000)        depth = 0x8000;
        else if (depth > 0x208000) depth = 0x208000;
        out[2] = depth;

        MEM32(DIVU_DVSR)   = depth;        /* divisor                       */
        MEM32(DIVU_DVDNTH) = 0x100;        /* dividend hi                   */
        MEM32(DIVU_DVDNTL) = 0;            /* dividend lo -> start 64/32    */
        recip = MEMU32(DIVU_DVDNTUL);      /* quotient (0x100<<32)/depth    */

        acc = (long long)M[4] * coord[0] + (long long)M[5] * coord[1] + (long long)M[6] * coord[2];
        ynum = (s32)(acc >> 16) + M[7];    /* row 1 */
        acc = (long long)M[8] * coord[0] + (long long)M[9] * coord[1] + (long long)M[10] * coord[2];
        xnum = (s32)(acc >> 16) + M[11];   /* row 2 */

        sx = (s32)(((long long)xnum * (s32)recip) >> 16) + 0x8000;
        sy = (s32)(((long long)ynum * (s32)recip) >> 16) + 0x8000;
        out[0] = ((s16)(sx >> 16)) << 1;   /* x, doubled for aspect */
        out[1] = (s16)(sy >> 16);          /* y */

        out += 4;
    }
}

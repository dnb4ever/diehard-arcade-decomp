/* 0x02213694  vertex_transform_project: transform `count` vertices by the 3x4 matrix
 * @0x060da040 and perspective-divide into the output array @0x060debc8 (16-byte stride).
 * Per vertex (src = 4 longs x,y,z,w; w ignored):
 *   row r result = ((s64)(M[4r]*x + M[4r+1]*y + M[4r+2]*z) >> 16) + M[4r+3]   (MAC 16.16 dot)
 *   scratch[0x6098b1c]=row2, [+4]=row1, [+8]=row0  (pre-dec stores from +0xc)
 *   depth z' = row0 result, clamped to [0x8000, 0x208000]; out[+8] = z'
 *   out[+0] = (row2<<4) / (z'>>4);  out[+4] = (row1<<4) / (z'>>4)   (SH-2 DIVU 32/32)
 * fidelity: faithful - clrmac/mac.l x3 dot per row, xtrct = >>16, +M[3] translation, cmp/gt
 *   clamp, DVSR/DVDNT/DVDNTUL divide loop (2x), 16-byte output stride, dt r8 outer loop. */
#include "dh_band21.h"

void vertex_transform_project(s32 count, s32 *verts) {
    s32 *out = PTR(s32, 0x060debc8);
    s32 *M   = PTR(s32, 0x060da040);
    s32 *tmp = PTR(s32, DH_TRANSFORMED_VECTOR_ADDR);   /* 3-long scratch: tmp[0]=row2, tmp[1]=row1, tmp[2]=row0 */
    s32 *src = verts;
    int n;

    for (n = 0; n < count; n++) {
        s32 x = *src++, y = *src++, z = *src++;
        long long acc;
        s32 zdepth, zdiv;
        int i;

        src++;  /* skip w */

        acc = (long long)M[0] * x + (long long)M[1] * y + (long long)M[2] * z;
        tmp[2] = (s32)(acc >> 16) + M[3];      /* row 0 */
        acc = (long long)M[4] * x + (long long)M[5] * y + (long long)M[6] * z;
        tmp[1] = (s32)(acc >> 16) + M[7];      /* row 1 */
        acc = (long long)M[8] * x + (long long)M[9] * y + (long long)M[10] * z;
        tmp[0] = (s32)(acc >> 16) + M[11];     /* row 2 */

        zdepth = tmp[2];
        out[2] = zdepth;
        if (0x8000 > zdepth) {                 /* cmp/gt r0,r3 (r3=0x8000) */
            out[2] = 0x8000;
            tmp[2] = 0x8000;
        } else if (zdepth > 0x208000) {
            tmp[2] = 0x208000;
            out[2] = 0x208000;
        }

        zdiv = (u32)tmp[2] >> 4;               /* shlr2 x2 (logical) */
        for (i = 0; i < 2; i++) {
            MEM32(DIVU_DVSR)  = zdiv;
            MEM32(DIVU_DVDNT) = tmp[i] << 4;   /* shll2 x2 */
            out[i] = MEM32(DIVU_DVDNTUL);
        }

        out += 4;   /* 16-byte stride */
    }
}

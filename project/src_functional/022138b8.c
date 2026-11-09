/* 0x022138b8  vertex_transform_project_m: like vertex_transform_project (0x213694)
 * but the 3x4 matrix base is passed in (arg3 = M); output array g_060dfbc8 (16-byte
 * stride), 3-long scratch @g_06098b5c. Also pre-clears 0x40 words @(M + 0x40000000).
 * No w-branch (w read and ignored). Per vertex (src = x,y,z,w):
 *   scratch[2]=row0, scratch[1]=row1, scratch[0]=row2   (pre-dec stores from +0xc)
 *     row r = ((s64 dot M[4r..4r+2]) >> 16) + M[4r+3]
 *   depth = row0, clamped to [0x8000, 0x208000]; out[2] = depth
 *   out[0] = (row2<<4) / (depth>>4);  out[1] = (row1<<4) / (depth>>4)  (DIVU 32/32)
 * fidelity: HIGH - clrmac/mac.l x3 per row (xtrct = >>16), cmp/gt clamp,
 *   DVSR/DVDNT/DVDNTUL 32/32 divide (2x), 16-byte stride, dt r8 outer loop. */
#include "dh_band21.h"

void vertex_transform_project_m(s32 count, s32 *verts, s32 *M) {
    s16 *zp      = (s16 *)((char *)(uintptr_t)M + 0x40000000);
    s32 *out     = PTR(s32, 0x060dfbc8);
    s32 *scratch = PTR(s32, 0x06098b5c);
    s32 *src     = verts;
    int i, n;

    for (i = 0; i < 0x40; i++)             /* zero 0x40 words @ M+0x40000000 */
        zp[i] = 0;

    for (n = count; n > 0; n--) {
        s32 x = *src++, y = *src++, z = *src++;
        long long acc;
        s32 depth, zdiv;

        src++;  /* w: read then ignored */

        acc = (long long)M[0] * x + (long long)M[1] * y + (long long)M[2] * z;
        scratch[2] = (s32)(acc >> 16) + M[3];     /* row 0 */
        acc = (long long)M[4] * x + (long long)M[5] * y + (long long)M[6] * z;
        scratch[1] = (s32)(acc >> 16) + M[7];     /* row 1 */
        acc = (long long)M[8] * x + (long long)M[9] * y + (long long)M[10] * z;
        scratch[0] = (s32)(acc >> 16) + M[11];    /* row 2 */

        depth = scratch[2];
        out[2] = depth;
        if (depth < 0x8000) {
            out[2] = 0x8000;
            scratch[2] = 0x8000;
        } else if (depth > 0x208000) {
            scratch[2] = 0x208000;
            out[2] = 0x208000;
        }

        zdiv = (u32)scratch[2] >> 4;
        for (i = 0; i < 2; i++) {
            MEM32(DIVU_DVSR)  = zdiv;
            MEM32(DIVU_DVDNT) = scratch[i] << 4;
            out[i] = MEM32(DIVU_DVDNTUL);
        }

        out += 4;   /* 16-byte stride */
    }
}

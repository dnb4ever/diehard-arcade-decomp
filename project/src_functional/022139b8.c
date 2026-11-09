/* 0x022139b8  project_sprite_quad: project the active 3x4 matrix's translation
 * column (S[3],S[7],S[11]) and a scaled (w,h) extent into a packed 3-long sprite
 * descriptor @g_06098b1c. S = *g_06098878 (the matrix set by vec3_matmul_out etc.).
 * args: (w, h, scale).  depth = S[11] clamped to >= 0x8000 (min only).
 *   recip = (0x100<<32) / depth                     (SH-2 DIVU 64/32)
 *   px = ((s32)((S[3]*recip)>>16)) + 0x8000;  py = ((s32)((S[7]*recip)>>16)) + 0x8000
 *   out[0] = ((((px>>16)<<1) & 0xffff) << 16) | ((u32)(-py) >> 16 & 0xffff)
 *            (screen x doubled in hi word, negated screen y in lo word)
 *   out[1] = depth
 *   ew = scale*w, eh = scale*h  (dmuls.l via <<16 / xtrct>>16)
 *   out[2] = ((mach(ew*recip)>>8 & 0xffff)<<16) | ((mach(eh*recip)<<8)>>16 & 0xffff)
 * fidelity: HIGH - clrmac/dmuls.l pairs, xtrct = middle-32 (>>16), mach = hi 32 of
 *   the 64-bit product, swap.w/neg/shll/shlr8/shll8 bit-packing modeled literally,
 *   DVSR/DVDNTH/DVDNTL 64/32 divide w/ DVDNTUL readback. */
#include "dh_band21.h"

void project_sprite_quad(s32 w, s32 h, s32 scale) {
    s32 *S   = (s32 *)(uintptr_t)MEMU32(DH_MATRIX_STACK_CURSOR_ADDR);
    s32 *out = PTR(s32, DH_TRANSFORMED_VECTOR_ADDR);
    s32 depth, px, py, ew, eh;
    u32 recip, machw, machh, r3, r5;
    long long p;

    depth = S[11];
    if (depth < 0x8000) depth = 0x8000;        /* min clamp only (cmp/gt) */

    MEM32(DIVU_DVSR)   = depth;
    MEM32(DIVU_DVDNTH) = 0x100;
    MEM32(DIVU_DVDNTL) = 0;                     /* start 64/32 divide */
    out[1] = depth;                            /* @(4,r2) */
    recip = MEMU32(DIVU_DVDNTUL);

    /* SHLL16 truncates to a word before DMULS.L. Scaled extents: (scale*(w<<16))>>16 == scale*w */
    ew = (s32)(((long long)scale * (s32)((u32)w << 16)) >> 16);
    eh = (s32)(((long long)scale * (s32)((u32)h << 16)) >> 16);

    px = (s32)(((long long)S[3] * (s32)recip) >> 16) + 0x8000;
    py = (s32)(((long long)S[7] * (s32)recip) >> 16) + 0x8000;

    /* out[0]: hi = (swap.w(px) << 1) low 16 == ((px>>16)<<1)&0xffff ; lo = (-py)>>16 */
    out[0] = (s32)(((u32)((((u32)px >> 16) << 1)) << 16) | (((u32)(-py) >> 16) & 0xffff));

    p = (long long)ew * (s32)recip;  machw = (u32)((unsigned long long)p >> 32);
    p = (long long)eh * (s32)recip;  machh = (u32)((unsigned long long)p >> 32);
    r3 = machw >> 8;                            /* shlr8 */
    r5 = machh << 8;                            /* shll8 */
    out[2] = (s32)(((r3 & 0xffff) << 16) | ((r5 >> 16) & 0xffff));   /* xtrct */
}

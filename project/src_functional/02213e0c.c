/* quad_test_visible [0x02213e0c]: Applies winding, depth and screen-bounds tests to a projected quad. */
/* 0x02213e0c  quad_clip_visible: decide whether the screen-space quad at `quad`
 * (eight s16: x0,y0,x1,y1,x2,y2,x3,y3) is a front-facing, on-screen, in-depth
 * face. Returns 1 if it should be drawn, else 0.
 *   - When `flag`==0, first reject back-facing quads: build one valid edge pair
 *     (choosing non-degenerate corners), then require the 2D cross product < 0.
 *   - Query the depth/validity via dispatch helper func_02213d90(sel, quad):
 *       if its result >= 0x50000, accept iff result in [0x10000, 0x1FBFFF];
 *       otherwise require the quad bounding box to overlap the screen rect
 *       (x in [-0x160,0x160], y in [-0x78,0x78]) and then range-check the
 *       stored depth quad[0x20] against [0x10000, 0x1FBFFF].
 * fidelity: faithful - muls.w cross (s16 operands), min/max via cmp/gt swaps,
 *   xor/and sign-bit box-overlap trick, dispatch through func_02213d90.
 *   sel arrives in r9 and quad in r8 at the dispatch, matching (sel, quad). */
#include "dh.h"

extern s32 quad_select_depth(s32 sel, void *obj);

#define Q(n) ((s16)M16(quad, (n) * 2))

s32 quad_test_visible(s32 flag, s32 sel, void *quad) {
    s32 result;

    if (flag == 0) {
        s16 q0 = Q(0), q1 = Q(1), q2 = Q(2), q3 = Q(3);
        s16 q4 = Q(4), q5 = Q(5), q6 = Q(6), q7 = Q(7);
        s32 e0, e1, e2, e3, cross;

        if (q0 == q2 && q1 == q3) {
            e0 = q2 - q4; e1 = q7 - q5; e2 = q6 - q4; e3 = q3 - q5;
        } else if (q2 == q4 && q3 == q5) {
            e0 = q4 - q6; e1 = q1 - q7; e2 = q0 - q6; e3 = q5 - q7;
        } else {
            e0 = q0 - q2; e1 = q5 - q3; e2 = q4 - q2; e3 = q1 - q3;
        }
        cross = (s16)e0 * (s16)e1 - (s16)e2 * (s16)e3;
        if (cross >= 0) {
            return 0;
        }
    }

    result = quad_select_depth(sel, quad);
    if (result >= 0x50000) {
        return (result >= 0x10000 && result <= 0x1fbfff) ? 1 : 0;
    } else {
        s16 q0 = Q(0), q1 = Q(1), q2 = Q(2), q3 = Q(3);
        s16 q4 = Q(4), q5 = Q(5), q6 = Q(6), q7 = Q(7);
        s32 minx, maxx, miny, maxy, lo, hi;

        if (q2 <= q0) { minx = q2; maxx = q0; } else { minx = q0; maxx = q2; }
        if (q6 <= q4) { lo = q6; hi = q4; }   else { lo = q4; hi = q6; }
        if (lo < minx) minx = lo;
        if (hi > maxx) maxx = hi;

        if (q3 <= q1) { miny = q3; maxy = q1; } else { miny = q1; maxy = q3; }
        if (q7 <= q5) { lo = q7; hi = q5; }   else { lo = q5; hi = q7; }
        if (lo < miny) miny = lo;
        if (hi > maxy) maxy = hi;

        if ((((maxy + 0x78) ^ (miny - 0x78)) & ((maxx + 0x160) ^ (minx - 0x160))) < 0) {
            result = M32(quad, 0x20);
            return (result >= 0x10000 && result <= 0x1fbfff) ? 1 : 0;
        }
        return 0;
    }
}

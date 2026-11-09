/* 0x02213390  interp_shorts_48: blend two 48-entry s16 tables toward a target.
 * Record selected by `rec` (stride 0x6c). For each of 48 shorts:
 *   delta = (s16)(b[i] - a[i]);  prod = ((s16)weight * delta) >> shift (logical);
 *   out[i] = (s16)(b[i] - prod).
 * a=@0x0609661c, b=@0x06096910, out=@0x06096328, each + rec*0x6c.
 * fidelity: faithful - clrmac/muls.w (16x16->32), inner shlr-by-`shift` loop (skipped when shift==0,
 *   equivalent to >>0), sub, mov.w store, 48-iteration outer dt loop. Leaf. */
#include "dh_band21.h"

void interp_shorts_48(s32 rec, s32 shift, s32 weight) {
    s16 *out = PTR(s16, 0x06096328 + rec * 0x6c);
    s16 *a   = PTR(s16, 0x0609661c + rec * 0x6c);
    s16 *b   = PTR(s16, 0x06096910 + rec * 0x6c);
    int i;
    for (i = 0; i < 48; i++) {
        s32 delta = (s16)(b[i] - a[i]);        /* muls.w uses low 16 bits */
        s32 prod  = (s16)weight * delta;        /* signed 16x16 -> 32      */
        prod = (u32)prod >> shift;              /* shlr repeated `shift` times (logical) */
        out[i] = (s16)(b[i] - prod);
    }
}

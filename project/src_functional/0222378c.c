#include "dh.h"
/* 0x0222378c  fmt_uint_base2: right-justified binary formatter into an output context.
 * out+4: pad flag (0 -> '0', else ' '); out+8: remaining field width; out+0xC: char
 * cursor (advanced per char); out+0x10: emitted count. Converts value to base-2 glyphs
 * via g_060755FB[0..1] (built least-significant first in a local buffer), left-pads to the
 * field width, then emits the glyphs most-significant first. A zero value emits the pad run
 * then a single '0'.
 * fidelity: HIGH - value/width/count 32-bit; the trailing width lands at -1 as in the ROM. */
extern s8 g_060755FB[];  /* binary digit glyphs */

#define OUT_PUTC(ctx, ch) do {                 \
    s8 *o_ = (s8 *)MP((ctx), 0xC);             \
    MP((ctx), 0xC) = o_ + 1;                   \
    *o_ = (ch);                   \
    M32((ctx), 0x10) += 1;                     \
} while (0)

void fmt_uint_base2(u32 value, s32 unused_r5, void *out) {
    (void)unused_r5; /* ROM uses r6 for the output context. */
    s8 buf[16];
    s8 *p;
    s32 ndig = 0;
    s8 pad = (M32(out, 4) == 0) ? 0x30 : 0x20;

    buf[14] = 0;
    p = &buf[13];
    if (value != 0) {
        do {
            *p-- = g_060755FB[value & 1];
            value >>= 1;
            ndig++;
        } while (value != 0);
        {
            s32 rem = M32(out, 8) - ndig;
            M32(out, 8) = rem;
            if (rem > 0) {
                s32 w;
                do {
                    w = M32(out, 8);
                    M32(out, 8) = w - 1;
                    if (w != 0)
                        OUT_PUTC(out, pad);
                } while (w != 0);
            }
        }
        for (;;) {
            p++;
            if (*p == 0)
                break;
            OUT_PUTC(out, *p);
        }
    } else {
        s32 w = M32(out, 8) - 1;
        M32(out, 8) = w;
        if (w > 0) {
            s32 x;
            do {
                x = M32(out, 8);
                M32(out, 8) = x - 1;
                if (x != 0)
                    OUT_PUTC(out, pad);
            } while (x != 0);
        }
        OUT_PUTC(out, 0x30);
    }
}

#include "dh.h"
/* 0x02223968  fmt_uint_base16: right-justified hexadecimal formatter into an output context.
 * Same context layout and padding as fmt_uint_base2 (out+4 pad flag, out+8 width, out+0xC
 * cursor, out+0x10 count). The glyph table is selected by upper: nonzero -> g_060755FE,
 * zero -> g_0607560F. Converts value to base-16 glyphs (built least-significant first) by
 * masking the low nibble and shifting right by 4, left-pads to the field width, then emits
 * most-significant first; a zero value emits the pad run then a single '0'.
 * fidelity: HIGH - value/width/count 32-bit; width lands at -1 as in the ROM. */
extern s8 g_060755FE[];  /* hex glyphs variant A */
extern s8 g_0607560F[];  /* hex glyphs variant B */

#define OUT_PUTC(ctx, ch) do {                 \
    s8 *o_ = (s8 *)MP((ctx), 0xC);             \
    MP((ctx), 0xC) = o_ + 1;                   \
    *o_ = (ch);                                \
    M32((ctx), 0x10) += 1;                     \
} while (0)

void fmt_uint_base16(u32 value, s32 upper, void *out) {
    s8 *tbl = (upper != 0) ? g_060755FE : g_0607560F;
    s8 buf[16];
    s8 *p;
    s32 ndig = 0;
    s8 pad = (M32(out, 4) == 0) ? 0x30 : 0x20;

    buf[14] = 0;
    p = &buf[13];
    if (value != 0) {
        do {
            *p-- = tbl[value & 0xF];
            value >>= 4;
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

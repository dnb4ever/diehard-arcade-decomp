/* format_decimal [0x02223862]: Formats signed or unsigned decimal digits and padding into the output context. */
#include "dh.h"
/* 0x02223862  signed/unsigned decimal formatter (%d / %u backend for 0x02223142 & 0x02223460).
 * arg0 (r4): value. arg1 (r5): signed flag (nonzero -> treat value as signed: emit '-' and
 *            use the magnitude). arg2 (r6): output context - +0x04 pad-mode (0 => pad with
 *            '0', else ' '), +0x08 field width (consumed in place), +0x0C write cursor,
 *            +0x10 running char count.
 * Digits are built backward into a local buffer with the unsigned divide/mod runtime helpers
 * (udiv32 = x/10 quotient, umod32 = x%10 remainder), then the optional sign and
 * left-padding are emitted, then the digits in order. Zero is a single '0' padded to width-1.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf -> 0x24a1c8 (umod), 0x24a120 (udiv).
 * The field-width field is decremented to -1 exactly as the ROM leaves it (caller resets it). */

extern u32 udiv32(u32 dividend, u32 divisor);  /* unsigned quotient (r1/r0 -> r0) */
extern u32 umod32(u32 dividend, u32 divisor);  /* unsigned remainder (r1/r0 -> r0) */

static void putc_ctx(void *ctx, char ch) {
    char *o = MP(ctx, 0x0C);
    MP(ctx, 0x0C) = o + 1;
    *o = ch;
    M32(ctx, 0x10) += 1;
}

void format_decimal(s32 value, s32 signflag, void *ctx) {
    char buf[16];
    char *p = &buf[14];
    s32 digits = 0;
    s8 sign = 0x20;
    char pad = (M32(ctx, 0x04) != 0) ? 0x20 : 0x30;

    if (signflag != 0 && ((u32)value & 0x80000000u) != 0) {
        value = ~value + 1;
        sign = 0x2D;
    }

    *p = 0;
    if (value != 0) {
        p--;
        do {
            *p-- = (char)(umod32((u32)value, 10) + 0x30);
            value = (s32)udiv32((u32)value, 10);
            digits++;
        } while (value != 0);

        if (sign == 0x2D)
            putc_ctx(ctx, sign);

        M32(ctx, 0x08) -= digits;
        if (M32(ctx, 0x08) > 0) {
            for (;;) {
                s32 pre = M32(ctx, 0x08);
                M32(ctx, 0x08) = pre - 1;
                if (pre == 0)
                    break;
                putc_ctx(ctx, pad);
            }
        }
        for (;;) {
            char ch = *++p;
            if (ch == 0)
                break;
            putc_ctx(ctx, ch);
        }
    } else {
        M32(ctx, 0x08) -= 1;
        if (M32(ctx, 0x08) > 0) {
            for (;;) {
                s32 pre = M32(ctx, 0x08);
                M32(ctx, 0x08) = pre - 1;
                if (pre == 0)
                    break;
                putc_ctx(ctx, pad);
            }
        }
        putc_ctx(ctx, 0x30);
    }
}

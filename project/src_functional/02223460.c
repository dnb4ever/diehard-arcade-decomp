/* text_printf_large_draft [0x02223460]: Large-font formatter with stack-passed variadic ABI; v1.09 unsupported ABI preserved. */
#include "dh.h"
#include <stdarg.h>
/* 0x02223460  formatted text printer (large font) - a printf that formats into a local
 * line buffer and renders it via text_write_big (0x02223080).
 * ABI: the first four register args (r4-r7) are unused; the format string is the first stack
 * argument (sp+0x140), followed by the varargs (each a 4-byte slot).  An emit context (local,
 * 0x14 bytes) carries the write cursor (+0x0C) into the line buffer and a char count (+0x10);
 * its +0x04/+0x08 (pad-mode / field width) are re-initialised {1,0} before every conversion.
 * Specifiers: %% literal, %d/%u decimal (0x02223862), %b (0x0222378c), %x/%X hex (0x02223968),
 * %s string (0x02223a52).  A leading '0' and up to two width digits are parsed but NOT applied
 * (dead stores in the ROM); three or more width digits, or an unknown specifier, abort with
 * func_02222f34(g_060755EC) and return 0.  '\n' terminates & flushes the current line via
 * func_02222f34 and restarts the buffer; end-of-string flushes via func_02223080.  Returns 0 always
 * (the ROM's return slot sp+0x11C is initialised to 0 and never updated).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. All bsr targets are direct (no PIC). */

extern u8   g_060755EC[];
extern void text_flush_large_line(void *buf);                   /* line flush (newline / error) */
extern s32  text_write_large(void *buf);                   /* text_write_big */
extern void format_decimal(s32 v, s32 is_signed, void *ec);  /* %d / %u */
extern void fmt_uint_base2(s32 v, s32 is_signed, void *ec);  /* %b */
extern void fmt_uint_base16(s32 v, s32 upper, void *ec);      /* %x / %X */
extern void format_padded_string(void *s, void *ec);               /* %s */

static void ec_putc(void *ec, char ch) {
    char *o = MP(ec, 0x0C);
    *o++ = ch;
    MP(ec, 0x0C) = o;
    M32(ec, 0x10) += 1;
}

static void ec_reset(void *ec) {
    M32(ec, 0x00) = 0;
    M32(ec, 0x04) = 1;
    M32(ec, 0x08) = 0;
}

s32 text_printf_large_draft(s32 a0, s32 a1, s32 a2, s32 a3, const char *fmt, ...) {
    u8 ec[0x14];
    char buf[256];
    va_list ap;
    char c;

    (void)a0; (void)a1; (void)a2; (void)a3;
    va_start(ap, fmt);

    MP(ec, 0x0C) = buf;
    buf[0] = 0;
    ec_reset(ec);
    M32(ec, 0x10) = 0;

    while ((c = *fmt++) != 0) {
        if (c == '%') {
            char conv = *fmt++;
            s32 ndig = 0;
            if (conv == '%') {
                ec_putc(ec, '%');
                continue;
            }
            if (conv != '0')
                fmt--;
            while ((conv = *fmt++) >= '0' && conv <= '9')
                ndig++;
            if (ndig >= 3) {
                text_flush_large_line(g_060755EC);
                va_end(ap);
                return 0;
            }
            switch (conv) {
            case 'd': format_decimal(va_arg(ap, s32), 1, ec); break;
            case 'u': format_decimal(va_arg(ap, s32), 0, ec); break;
            case 'b': fmt_uint_base2(va_arg(ap, s32), 0, ec); break;
            case 'x': fmt_uint_base16(va_arg(ap, s32), 0, ec); break;
            case 'X': fmt_uint_base16(va_arg(ap, s32), 1, ec); break;
            case 's': format_padded_string(va_arg(ap, void *), ec); break;
            default:
                text_flush_large_line(g_060755EC);
                va_end(ap);
                return 0;
            }
            ec_reset(ec);
        } else if (c == '\n') {
            *(char *)MP(ec, 0x0C) = 0;
            MP(ec, 0x0C) = (char *)MP(ec, 0x0C) + 1;
            text_flush_large_line(buf);
            MP(ec, 0x0C) = buf;
            buf[0] = 0;
            ec_reset(ec);
        } else {
            ec_putc(ec, c);
        }
    }

    *(char *)MP(ec, 0x0C) = 0;
    MP(ec, 0x0C) = (char *)MP(ec, 0x0C) + 1;
    text_write_large(buf);
    va_end(ap);
    return 0;
}

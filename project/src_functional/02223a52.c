/* format_padded_string [0x02223a52]: Pads a string to the context's width and advances its output cursor and count. */
#include "dh.h"
/* 0x02223a52  right-justify a string into an output buffer (p+0xC = write cursor, p+0x10 = count),
 * padding to field width p+0x8 with '0' (0x30) when p+0x4==0 else ' ' (0x20), then append the string.
 * fidelity: HIGH - exact; p+0xC is a char* cursor, p+0x4/0x8/0x10 are 32-bit. */
void format_padded_string(s8 *str, void *p) {
    s32 n = 0;
    if (M32(p, 0x8) != 0) {
        s8 *q = str;
        s8 pad = (M32(p, 0x4) == 0) ? 0x30 : 0x20;
        while (*q++ != 0) {
            n++;
        }
        {
            s32 rem = M32(p, 0x8) - n;
            M32(p, 0x8) = rem;
            if (rem > 0) {
                for (;;) {
                    s32 d = M32(p, 0x8) - 1;
                    M32(p, 0x8) = d;
                    if ((d + 1) == 0) {
                        break;
                    }
                    {
                        s8 *w = (s8 *)MP(p, 0xC);
                        MP(p, 0xC) = w + 1;
                        *w = pad;
                        M32(p, 0x10) = M32(p, 0x10) + 1;
                    }
                }
            }
        }
    }
    {
        s8 c;
        while ((c = *str++) != 0) {
            s8 *w = (s8 *)MP(p, 0xC);
            MP(p, 0xC) = w + 1;
            *w = c;
            M32(p, 0x10) = M32(p, 0x10) + 1;
        }
    }
}

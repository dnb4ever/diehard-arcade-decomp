/* tilemap_copy_wrap_wide [0x0221675e]: Copies biased pattern-name cells with 64-cell page seams and wrapping. */
#include "dh.h"
/* 0x0221675e (tilemap_copy_wrap_w80) - copy a desc->cols x desc->rows block of
 * 16-bit cells from desc->src into a tilemap at dest=arg1, adding (arg0+desc->bias)
 * to each cell. Columns wrap at the cell width (0x80 if arg5&0x200 else 0x40) and
 * rows wrap at the cell height (0x80 if arg5&0x400 else 0x40); the starting column
 * (arg3) and row (arg4) are taken modulo those sizes. Within a 0x40-wide half the
 * cell maps linearly; the far half is offset by +0x1F80, and a row step crossing the
 * 0x40 boundary uses +0x2080 (else +0x80 bytes). Row stride is 0x80 bytes (<<7).
 * fidelity: HIGH - cells s16 (+bias), counts desc->cols/rows s8, wrap by masks. */
typedef struct { s8 cols; s8 rows; s16 bias; s16 *src; } BlitDesc;

void tilemap_copy_wrap_wide(s32 arg0, void *arg1, void *arg2, s32 arg3, s32 arg4, s32 arg5) {
    BlitDesc *d = (BlitDesc *)arg2;
    s32 bias = arg0 + d->bias;
    s16 *src = d->src;
    s32 cw = (arg5 & 0x200) ? 0x80 : 0x40;
    s32 col0 = arg3 & (cw - 1);
    s32 ch = (arg5 & 0x400) ? 0x80 : 0x40;
    s32 row0 = arg4 & (ch - 1);
    s16 *p = (s16 *)((char *)arg1 + col0 * 2 + (row0 << 7));
    s32 rowstep = cw * 2;
    s16 *colbase = (s16 *)((char *)arg1 + col0 * 2);
    s32 oy = row0;
    s32 ry = 0;

    while (ry < d->rows) {
        s16 *rowp = p;
        s32 cx = col0;
        s32 rx = 0;
        while (rx < d->cols) {
            if (cx < cw) {
                if (cx < 0x40) {
                    *p = *src + bias;
                    src++;
                    p++;
                } else {
                    s16 v = *src;
                    s16 *q = (s16 *)((char *)p + 0x1F80);
                    src++;
                    p++;
                    *q = v + bias;
                }
            } else {
                cx = 0;
                p = (s16 *)((char *)p - rowstep);
                *p = *src + bias;
                src++;
                p++;
            }
            rx++;
            cx++;
        }
        oy++;
        if (oy < ch) {
            p = (s16 *)((char *)rowp + ((oy != 0x40) ? 0x80 : 0x2080));
        } else {
            p = colbase;
            oy = 0;
        }
        ry++;
    }
}

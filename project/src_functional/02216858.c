/* tilemap_copy_wrap_narrow [0x02216858]: Copies biased pattern-name cells with thirty-two-cell page seams and wrapping. */
#include "dh.h"
/* 0x02216858 (tilemap_copy_wrap_w40) - narrow-cell sibling of tilemap_copy_wrap_w80
 * (0x0221675e). Copies desc->cols x desc->rows 16-bit cells from desc->src into
 * dest=arg1, adding (arg0+desc->bias). Cell width is 0x40 (if arg5&0x200) else 0x20,
 * height 0x40 (if arg5&0x400) else 0x20; linear within a 0x20-wide half, far half
 * offset +0x7C0, row-step across the 0x20 boundary +0x840 (else +0x40). Row stride
 * is 0x40 bytes (<<6).
 * fidelity: HIGH - cells s16 (+bias), counts desc->cols/rows s8, wrap by masks. */
typedef struct { s8 cols; s8 rows; s16 bias; s16 *src; } BlitDesc;

void tilemap_copy_wrap_narrow(s32 arg0, void *arg1, void *arg2, s32 arg3, s32 arg4, s32 arg5) {
    BlitDesc *d = (BlitDesc *)arg2;
    s32 bias = arg0 + d->bias;
    s16 *src = d->src;
    s32 cw = (arg5 & 0x200) ? 0x40 : 0x20;
    s32 col0 = arg3 & (cw - 1);
    s32 ch = (arg5 & 0x400) ? 0x40 : 0x20;
    s32 row0 = arg4 & (ch - 1);
    s16 *p = (s16 *)((char *)arg1 + col0 * 2 + (row0 << 6));
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
                if (cx < 0x20) {
                    *p = *src + bias;
                    src++;
                    p++;
                } else {
                    s16 v = *src;
                    s16 *q = (s16 *)((char *)p + 0x7C0);
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
            p = (s16 *)((char *)rowp + ((oy != 0x20) ? 0x40 : 0x840));
        } else {
            p = colbase;
            oy = 0;
        }
        ry++;
    }
}

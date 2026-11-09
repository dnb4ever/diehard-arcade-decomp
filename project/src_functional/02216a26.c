/* tilemap_fill_narrow [0x02216a26]: Fills a wrapping tile rectangle across thirty-two-cell page seams. */
#include "dh.h"
#include <stdint.h>
/* 0x02216a26 (tilemap_fill_40) - sibling of tilemap_fill_80 (0x0221694e) for the
 * narrower 0x40/0x20-wide tilemap pages: w = (flags&0x200)?0x40:0x20,
 * h = (flags&0x400)?0x40:0x20. Fills an ncols x nrows rectangle with a constant 16-bit
 * tile starting at base + (cx*2) + (cy<<6); columns wrap at the cell edge, the second
 * half (col>=0x20) writes the neighbouring page at +0x7C0, and rows advance +0x40,
 * +0x840 at the page seam or wrap to the column base. fidelity: HIGH - strides/seams
 * verified against the asm. */
void tilemap_fill_narrow(s16 tile, s32 base, s32 colseed, s32 rowseed, s32 ncols, s32 nrows, s32 flags) {
    s32 w = (flags & 0x200) ? 0x40 : 0x20;
    s32 cx = colseed & (w - 1);
    s32 h = (flags & 0x400) ? 0x40 : 0x20;
    s32 cy = rowseed & (h - 1);
    s16 *p = (s16 *)(uintptr_t)(base + cx * 2 + (cy << 6));
    s32 rowstep = w * 2;
    s16 *colbase = (s16 *)(uintptr_t)(base + cx * 2);
    s32 oy = cy, yy = 0;

    while (yy < nrows) {
        s16 *rowp = p;
        s32 cxx = cx, xx = 0;
        if (ncols > 0) {
            do {
                if (cxx < w) {
                    if (cxx < 0x20)
                        *p = tile;
                    else
                        *(s16 *)((char *)p + 0x7C0) = tile;
                    p++;
                } else {
                    cxx = 0;
                    p = (s16 *)((char *)p - rowstep);
                    *p = tile;
                    p++;
                }
                xx++;
                cxx++;
            } while (xx < ncols);
        }
        oy++;
        if (oy < h)
            p = (s16 *)((char *)rowp + ((oy != 0x20) ? 0x40 : 0x840));
        else {
            p = colbase;
            oy = 0;
        }
        yy++;
    }
}

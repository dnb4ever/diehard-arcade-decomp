/* tilemap_fill_paged80 [0x0221694e]: Flag-selected 64/128 dimensions with horizontal/vertical page seams and wrapping. */
#include "dh.h"
#include <stdint.h>
/* 0x0221694e (tilemap_fill_80) - fill an ncols x nrows rectangle of a VDP2 tilemap
 * with a constant 16-bit tile. Cell page is 0x80 or 0x40 wide per flags 0x200/0x400:
 * w = (flags&0x200)?0x80:0x40, h = (flags&0x400)?0x80:0x40. Start at base + (cx*2) +
 * (cy<<7) where cx=colseed&(w-1), cy=rowseed&(h-1). Columns wrap at the cell edge
 * (rewinding one row, w*2 bytes); the second horizontal half (col>=0x40) writes to the
 * neighbouring page at +0x1F80. Rows advance +0x80, +0x2080 at the 0x40 page seam, or
 * wrap to the column base. Sibling of tilemap_fill_40 (0x02216a26) and tilemap_blit_add
 * (0x02216714). fidelity: HIGH - strides, seams and wrap verified against the asm. */
void tilemap_fill_paged80(s16 tile, s32 base, s32 colseed, s32 rowseed, s32 ncols, s32 nrows, s32 flags) {
    s32 w = (flags & 0x200) ? 0x80 : 0x40;
    s32 cx = colseed & (w - 1);
    s32 h = (flags & 0x400) ? 0x80 : 0x40;
    s32 cy = rowseed & (h - 1);
    s16 *p = (s16 *)(uintptr_t)(base + cx * 2 + (cy << 7));
    s32 rowstep = w * 2;
    s16 *colbase = (s16 *)(uintptr_t)(base + cx * 2);
    s32 oy = cy, yy = 0;

    while (yy < nrows) {
        s16 *rowp = p;
        s32 cxx = cx, xx = 0;
        if (ncols > 0) {
            do {
                if (cxx < w) {
                    if (cxx < 0x40)
                        *p = tile;
                    else
                        *(s16 *)((char *)p + 0x1F80) = tile;
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
            p = (s16 *)((char *)rowp + ((oy != 0x40) ? 0x80 : 0x2080));
        else {
            p = colbase;
            oy = 0;
        }
        yy++;
    }
}

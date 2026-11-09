/* 0x2216a26  tile_blit_40: sibling of f_21694e for the narrower 0x20/0x40-wide
   tilemap cells (per arg6 flags); fills a region with 16-bit tile arg0, wrapping
   columns at the cell edge and rows via the 0x7C0/0x40/0x840 strides.
   args: (s16 tile, s32 base, s32 colseed, s32 rowseed, s32 ncols, s32 nrows, s32 flags)
   NONMATCHING ~5.9%: functionally correct; same SHC r32 register-allocation version
   trait as f_21694e (7 callee-saved regs + spilled locals allocated differently than
   the 1996 ROM build). */
typedef short s16;
typedef int s32;

void f_216a26(s16 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s32 w, h, cx, cy, oy, yy, xx, cxx, rowstep;
    s16 *p, *rowp, *colbase;

    w = (arg6 & 0x200) ? 0x40 : 0x20;
    cx = arg2 & (w - 1);
    h = (arg6 & 0x400) ? 0x40 : 0x20;
    cy = arg3 & (h - 1);
    p = (s16 *)(arg1 + cx * 2 + (cy << 6));
    rowstep = w * 2;
    colbase = (s16 *)(arg1 + cx * 2);
    oy = cy;
    yy = 0;
    while (yy < arg5) {
        rowp = p;
        cxx = cx;
        xx = 0;
        if (arg4 > 0) {
            do {
                if (cxx < w) {
                    if (cxx < 0x20) {
                        *p = arg0;
                        p++;
                    } else {
                        s16 *old = p;
                        p++;
                        *(s16 *)((char *)old + 0x7C0) = arg0;
                    }
                } else {
                    cxx = 0;
                    p = (s16 *)((char *)p - rowstep);
                    *p = arg0;
                    p++;
                }
                xx++;
                cxx++;
            } while (xx < arg4);
        }
        oy++;
        if (oy < h) {
            p = (s16 *)((char *)rowp + ((oy != 0x20) ? 0x40 : 0x840));
        } else {
            oy = 0;
            p = colbase;
        }
        yy++;
    }
}

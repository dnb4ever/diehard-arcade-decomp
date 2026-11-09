/* 0x221694e  tile_blit_80: fill a rectangular region of a tilemap (0x40/0x80-wide
   cells per arg6 flags 0x200/0x400) with 16-bit tile value arg0, wrapping columns
   at the cell boundary and rows via the 0x1F80/0x2080 strides.
   args: (s16 tile, s32 base, s32 colseed, s32 rowseed, s32 ncols, s32 nrows, s32 flags)
   NONMATCHING ~6.5%: functionally correct, but SHC r32 register allocation diverges
   from the 1996 ROM build. ROM keeps `cx` in caller-saved r6 (leaf, no calls) and
   uses a 3-slot (-12) frame; r32 spills cx into a 4-slot (-16) frame and shuffles
   all 7 callee-saved regs. Allocation-order version trait, not coercible via source. */
typedef short s16;
typedef int s32;

void f_21694e(s16 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6) {
    s32 w, h, cx, cy, oy, yy, xx, cxx, rowstep;
    s16 *p, *rowp, *colbase;

    w = (arg6 & 0x200) ? 0x80 : 0x40;
    cx = arg2 & (w - 1);
    h = (arg6 & 0x400) ? 0x80 : 0x40;
    cy = arg3 & (h - 1);
    p = (s16 *)(arg1 + cx * 2 + (cy << 7));
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
                    if (cxx < 0x40) {
                        *p = arg0;
                    } else {
                        *(s16 *)((char *)p + 0x1F80) = arg0;
                    }
                    p++;
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
            p = (s16 *)((char *)rowp + ((oy != 0x40) ? 0x80 : 0x2080));
        } else {
            p = colbase;
            oy = 0;
        }
        yy++;
    }
}

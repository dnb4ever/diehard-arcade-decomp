/* text_write_small [0x02222fe0]: Writes a NUL-terminated string to the small-font tilemap with cursor wrapping. */
#include "dh.h"
/* 0x02222fe0  text_write_tilemap: emit a NUL-terminated string into the VDP2 text nametable
 * at 0x060997B0, advancing a persistent cursor (column 0x060997A4, block 0x060997A8). Each glyph
 * becomes a 16-bit tile entry: attribute word g_060756C8[current palette] OR the (sign-extended)
 * character code. The cursor wraps at column 64 (second half of the row, +0x1F80 bytes), at
 * column 128 (next block, back one full row) and at block 32 (back to the buffer start).
 * Returns the number of glyphs written.
 * fidelity: HIGH - byte-level pointer arithmetic preserved; cursor state matches the ROM. */
extern s32  g_ui_cursor_column;     /* cursor column */
extern s32  g_ui_cursor_row;     /* cursor block/row */
extern s32  g_060997AC;     /* current palette index */
extern u32  g_060756C8[];   /* per-palette tile attribute words */
extern u8   g_060997B0[];   /* VDP2 text nametable (16-bit tile entries) */
s32 text_write_small(const s8 *str) {
    s32 count = 0;
    s32 col = g_ui_cursor_column;
    s32 blk = g_ui_cursor_row;
    u8 *p = g_060997B0 + (((blk & 0x1F) << 7) + ((col & 0x3F) << 1));
    s8 c;

    if (col >= 64) {
        p += 0x2000;
    }
    while ((c = *str++) != 0) {
        M16(p, 0) = (s16)(g_060756C8[g_060997AC] | c);
        p += 2;
        col = g_ui_cursor_column + 1;
        g_ui_cursor_column = col;
        count++;
        if (col == 64) {
            p += 0x1F80;
        } else if (col == 128) {
            g_ui_cursor_column = 0;
            p -= 0x2000;
            blk = g_ui_cursor_row + 1;
            g_ui_cursor_row = blk;
            if (blk >= 32) {
                p = g_060997B0;
                g_ui_cursor_row = 0;
            }
        }
    }
    return count;
}

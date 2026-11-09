/* text_write_large [0x02223080]: Writes a NUL-terminated string to the large-font tilemap with cursor wrapping. */
#include "dh.h"
/* 0x02223080  text_write_tilemap_big: large-font sibling of func_02222fe0. Emits a NUL-terminated
 * string into the VDP2 nametable at 0x060997B0, cursor state in 0x060997A4 (column) / 0x060997A8
 * (block). Tile entry = attribute word g_060756C8[palette] OR (char + 0x0C00 tile base). The big
 * font uses 32-column half-rows (wrap +0x7C0 bytes at col 32), 64-column rows (next block at col 64)
 * and 16 blocks. Returns glyph count.
 * fidelity: HIGH - byte-level pointer math and cursor wrapping verified against the ROM. */
extern s32  g_ui_cursor_column;     /* cursor column */
extern s32  g_ui_cursor_row;     /* cursor block/row */
extern s32  g_060997AC;     /* current palette index */
extern u32  g_060756C8[];   /* per-palette tile attribute words */
extern u8   g_060997B0[];   /* VDP2 text nametable (16-bit tile entries) */
s32 text_write_large(const s8 *str) {
    s32 count = 0;
    s32 col = g_ui_cursor_column;
    s32 blk = g_ui_cursor_row;
    u8 *p = g_060997B0 + (((blk & 0xF) << 6) + ((col & 0x1F) << 1));
    s8 c;

    if (col >= 0x20) {
        p += 0x800;
    }
    while ((c = *str++) != 0) {
        M16(p, 0) = (s16)(g_060756C8[g_060997AC] | ((u8)c + 0xC00));
        p += 2;
        col = g_ui_cursor_column + 1;
        g_ui_cursor_column = col;
        count++;
        if (col == 0x20) {
            p += 0x7C0;
        } else if (col == 0x40) {
            p -= 0x800;
            g_ui_cursor_column = 0;
            blk = g_ui_cursor_row + 1;
            g_ui_cursor_row = blk;
            if (blk >= 0x10) {
                g_ui_cursor_row = 0;
                p = g_060997B0;
            }
        }
    }
    return count;
}

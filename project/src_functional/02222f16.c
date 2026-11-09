/* text_flush_small_line [0x02222f16]: Writes small-font line, clears column and advances row modulo 64. */
#include "dh.h"
/* 0x02222f16  text line flush (normal font): text_write_tilemap(str) (bsr 0x222fe0,
 * r4 passed through), then reset cursor x 0x060997A4 = 0 and advance the row ring
 * 0x060997A8 = (row + 1) & 0x3F; return the call result.
 * Sibling of func_02222f34 (other font, mod 32). Called by func_02223142 on '\n'
 * and on format errors.
 * fidelity: HIGH - exact (hand-traced vs asm 0x222f16..0x222f32). */
extern s32 text_write_small(const s8 *str);
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
s32 text_flush_small_line(const s8 *str) {
    s32 r = text_write_small(str);
    g_ui_cursor_column = 0;
    g_ui_cursor_row = (g_ui_cursor_row + 1) & 0x3F;
    return r;
}

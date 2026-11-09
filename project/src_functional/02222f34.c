/* text_flush_large_line [0x02222f34]: Writes large-font line, clears column and advances row modulo 32. */
#include "dh.h"
/* 0x02222f34  call func_02223080, reset 0x060997A4, advance ring index 0x060997A8 (mod 32); return the call result.
 * fidelity: HIGH - exact (bsr 0x223080). */
extern s32 text_write_large(void);
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
s32 text_flush_large_line(void) {
    s32 r = text_write_large();
    g_ui_cursor_column = 0;
    g_ui_cursor_row = (g_ui_cursor_row + 1) & 0x1F;
    return r;
}

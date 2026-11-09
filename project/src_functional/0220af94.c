/* debug_print_table_record [0x0220af94]: Byte-truncated index selects 0x14-byte 060512a4 record; prints five words. */
#include "dh.h"
/* 0x0220af94 (HUD debug text: print a 5-field record from table g_060512A4) - selects
 * record `arg2` in the 0x14-byte-stride table at 0x060512A4 (index truncated to a byte:
 * a = 0x060512A4 + (u8)(arg2*0x14)) and prints its five consecutive 32-bit fields with
 * the shared formatter *g_060902A0(fmt, value), advancing the row (g_060997A8) each line
 * and keeping the column (g_060997A4 = col).
 * fidelity: HIGH - col/row u16 (extu.w); arg2 s16; index byte-truncated (extu.b);
 * fields 4 bytes apart; print thunk takes stack args (varargs: fmt first). Verified vs asm. */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, ...);

void debug_print_table_record(u16 col, u16 row, s16 arg2) {
    s32 a = 0x060512A4 + (u8)(arg2 * 0x14);

    g_ui_cursor_column = col; g_ui_cursor_row = row;     g_060902A0(0x060514FC, a);
    g_ui_cursor_column = col; g_ui_cursor_row = row + 1; g_060902A0(0x060514FC, a + 4);
    g_ui_cursor_column = col; g_ui_cursor_row = row + 2; g_060902A0(0x060514FC, a + 8);
    g_ui_cursor_column = col; g_ui_cursor_row = row + 3; g_060902A0(0x060514FC, a + 0xC);
    g_ui_cursor_column = col; g_ui_cursor_row = row + 4; g_060902A0(0x060514FC, a + 0x10);
}

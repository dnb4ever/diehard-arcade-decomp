/* debug_print_input_records [0x0220ade6]: Prints five 0x18-stride input button words after header using HUD formatter. */
#include "dh.h"
/* 0x0220ade6 (HUD debug text: dump 6 per-slot u16 values from g_06098734) - sets the
 * text cursor (g_060997A4 = column `col`, g_060997A8 = row `row`+i) and calls the
 * formatted-print thunk *g_060902A0(fmt[, value]) six times. The first line prints a
 * header string with no value; lines 1..5 print the u16 at g_06098734 + i*0x18.
 * fidelity: HIGH - col/row are u16 (extu.w); values 16-bit (mov.w/extu.w); the print
 * thunk takes its args on the stack (modelled as varargs: fmt first). Verified vs asm. */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, ...);
extern s16 g_player_input_records;

void debug_print_input_records(u16 col, u16 row) {
    g_ui_cursor_column = col;
    g_ui_cursor_row = row;
    g_060902A0(0x06051490);

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 1;
    g_060902A0(0x06051498, MU16(&g_player_input_records, 0x00));

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 2;
    g_060902A0(0x060514A0, MU16(&g_player_input_records, 0x18));

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 3;
    g_060902A0(0x060514A8, MU16(&g_player_input_records, 0x30));

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 4;
    g_060902A0(0x060514B0, MU16(&g_player_input_records, 0x48));

    g_ui_cursor_column = col;
    g_ui_cursor_row = row + 5;
    g_060902A0(0x060514B8, MU16(&g_player_input_records, 0x60));
}

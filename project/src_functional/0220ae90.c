/* debug_print_panel_records [0x0220ae90]: Refreshes 060987d8 and prints four panel records plus indexed 060906dc values. */
#include "dh.h"
/* 0x0220ae90 (HUD debug text: dump the 4 collision/zone records at g_060987D8) - prints
 * two header lines, calls the refresh helper *g_06000660(&g_060987D8), then for each of
 * four 4-byte records prints its bytes [0],[1],[2] plus a looked-up 32-bit value
 * (g_060906DC[ record[3] ]) via *g_060902A0, one row per record. A final line prints the
 * fifth record's bytes [0],[1] with a different format.
 * fidelity: HIGH - col/row u16 (extu.w); record fields u8 (extu.b); table index scaled
 * *4; the two-way-unrolled source loop is modelled as a 4-iteration loop (identical row
 * = base+i+2 each pass). Print thunk takes stack args (varargs: fmt first). Verified vs asm. */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern void (*g_060902A0)(s32, ...);
extern void (*g_06000660)(s32);
extern u8  g_060987D8;
extern s32 g_060906DC;

void debug_print_panel_records(u16 col, u16 row) {
    s32 i;

    g_ui_cursor_column = col; g_ui_cursor_row = row;     g_060902A0(0x060514C0);
    g_ui_cursor_column = col; g_ui_cursor_row = row + 1; g_060902A0(0x060514CC);
    g_06000660(0x060987D8);

    for (i = 0; i < 4; i++) {
        u8 *p = &g_060987D8 + i * 4;
        g_ui_cursor_column = col;
        g_ui_cursor_row = row + i + 2;
        g_060902A0(0x060514D8, i + 1, p[0], p[1], p[2], (&g_060906DC)[p[3]]);
    }

    {
        u8 *p = &g_060987D8 + i * 4;   /* i == 4: the fifth record */
        g_ui_cursor_column = col;
        g_ui_cursor_row = row + 6;
        g_060902A0(0x060514EC, p[0], p[1]);
    }
}

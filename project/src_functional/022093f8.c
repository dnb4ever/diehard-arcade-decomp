/* results_draw_header [0x022093f8]: Draws the selected results header and footer text rows using the HUD callbacks. */
#include "dh.h"

/* 0x022093f8 - func_022093f8: results/HUD header rows (shared helper of the 0x02209188 results-task family).
 * Split out of the former multi-function file 02209188.c (one function per
 * file); see 02209188.c for the family overview, the dispatcher, and the HUD
 * thunk conventions (g_060997A4/A8/AC cursor, *g_0609029C glyph, *g_060902A0
 * printf, *g_06000644 resident get-field). */

extern s32   g_ui_cursor_column;                                 /* cursor color/x */
extern s32   g_ui_cursor_row;                                 /* cursor mode/y  */
extern s32   g_060997AC;                                 /* cursor font    */
extern void (*g_0609029C)(void *desc);                   /* ui_glyph thunk */
extern void (*g_06090294)(void);                         /* ui/display init thunk (state 0) */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))

/* ====================== shared HUD header (0x2093f8) ====================== */
/* Draws the three fixed header rows (title/subtitle) for a results screen, using
 * the per-mode string tables selected by `code` (the task state byte). */
void results_draw_header(s32 code, void *ent)
{
    s32 idx = (s8)code * 4;

    if (M32(ent, 0x44) != 0) {
        g_06090294();                      /* re-run display init */
        M32(ent, 0x44) = 0;
    }
    g_060997AC = 5;
    g_ui_cursor_column = 9;
    g_ui_cursor_row = 2;
    g_0609029C(RMP(0x06090464 + idx));
    g_060997AC = 3;
    g_ui_cursor_column = 9;
    g_ui_cursor_row = 23;
    g_0609029C(RMP(0x06090498 + idx));
    g_ui_cursor_column = 9;
    g_ui_cursor_row = 25;
    g_0609029C(RMP(0x060904CC + idx));
    g_060997AC = 1;
}
/* fidelity: HIGH - hand-traced vs fn.py disassembly (0x2093f8..0x20947e). */

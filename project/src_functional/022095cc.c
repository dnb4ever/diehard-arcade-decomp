/* results_draw_state2_markers [0x022095cc]: State-2 nine-row labels and per-player flag marker grid. */
#include "dh_work.h"
#include "dh.h"

/* 0x022095cc - func_022095cc: results task state 2.
 * Split out of the former multi-function file 02209188.c (one function per
 * file); see 02209188.c for the family overview, the dispatcher, and the HUD
 * thunk conventions (g_060997A4/A8/AC cursor, *g_0609029C glyph, *g_060902A0
 * printf, *g_06000644 resident get-field). */

extern s32   g_ui_cursor_column;                                 /* cursor color/x */
extern s32   g_ui_cursor_row;                                 /* cursor mode/y  */
extern s32   g_060997AC;                                 /* cursor font    */
extern void (*g_0609029C)(void *desc);                   /* ui_glyph thunk */
extern void results_draw_header(s32 code, void *ent);  /* 0x022093f8 results header */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))

/* ========================= state 2 (0x2095cc) ========================= */
/* 9-row label block, event transition, then a per-flag grid of "marker" glyphs
 * for the two player records (0x06098734 / 0x0609874C) and the control struct. */
void results_draw_state2_markers(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32   i;
    uintptr_t tbl;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 6, tbl = 0x06090518; i < 24; i += 6, tbl += 12) {
        g_ui_cursor_column = 8; g_ui_cursor_row = i;     g_0609029C(RMP(tbl + 0));
        g_ui_cursor_column = 8; g_ui_cursor_row = i + 2; g_0609029C(RMP(tbl + 4));
        g_ui_cursor_column = 8; g_ui_cursor_row = i + 4; g_0609029C(RMP(tbl + 8));
    }

    if (RU16(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 1;
        M32(ent, 0x40) = 2;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 2;
    /* Per-flag markers: left column (x=0x16) = P1 (0x06098734), right column
     * (x=0x21) = P2 (0x0609874C), last row = control struct (0x06098794).  Each
     * flag word is re-read before every test (after each glyph call), as in ROM.
     * masks 0x80 come from `tst #0x80` (zero-extended; m2c's ~0x7F is wrong). */
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x80)  { g_ui_cursor_column = 0x16; g_ui_cursor_row = 8;    g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x80)  { g_ui_cursor_column = 0x21; g_ui_cursor_row = 8;    g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x40)  { g_ui_cursor_column = 0x16; g_ui_cursor_row = 0xA;  g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x40)  { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0xA;  g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x100) { g_ui_cursor_column = 0x16; g_ui_cursor_row = 0xC;  g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x100) { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0xC;  g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x200) { g_ui_cursor_column = 0x16; g_ui_cursor_row = 0xE;  g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x200) { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0xE;  g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x1)   { g_ui_cursor_column = 0x16; g_ui_cursor_row = 0x10; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x1)   { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0x10; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x2)   { g_ui_cursor_column = 0x16; g_ui_cursor_row = 0x12; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x2)   { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0x12; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(DH_PLAYER_INPUT_RECORDS_ADDR) & 0x4)   { g_ui_cursor_column = 0x16; g_ui_cursor_row = 0x14; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x0609874C) & 0x4)   { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0x14; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x06098794) & 0x8)   { g_ui_cursor_column = 0x11; g_ui_cursor_row = 0x16; g_0609029C((void *)(uintptr_t)0x06051430); }
    if (RU16(0x06098794) & 0x4)   { g_ui_cursor_column = 0x21; g_ui_cursor_row = 0x16; g_0609029C((void *)(uintptr_t)0x06051430); }

    g_060997AC = 1;
}
/* fidelity: HIGH - hand-traced vs fn.py disassembly (0x2095cc..0x20980c). The
 * two 0x80 masks are the corrected (zero-extended) `tst #0x80` forms. */

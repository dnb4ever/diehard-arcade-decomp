/* results_draw_state1_labels [0x022094c4]: State-1 six-row label block, transition and highlight glyph. */
#include "dh_work.h"
#include "dh.h"

/* 0x022094c4 - func_022094c4: results task state 1.
 * Split out of the former multi-function file 02209188.c (one function per
 * file); see 02209188.c for the family overview, the dispatcher, and the HUD
 * thunk conventions (g_060997A4/A8/AC cursor, *g_0609029C glyph, *g_060902A0
 * printf, *g_06000644 resident get-field). */

extern s32   g_ui_cursor_column;                                 /* cursor color/x */
extern s32   g_ui_cursor_row;                                 /* cursor mode/y  */
extern void (*g_0609029C)(void *desc);                   /* ui_glyph thunk */
extern void results_draw_header(s32 code, void *ent);  /* 0x022093f8 results header */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))
#define RS32(a) (*(s32  *)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))

/* ========================= state 1 (0x2094c4) ========================= */
/* 6-row label block, then an event-driven transition, then a highlight glyph. */
void results_draw_state1_labels(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32   i;
    uintptr_t tbl;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 6, tbl = 0x06090500; i < 18; i += 6, tbl += 12) {
        g_ui_cursor_column = 12; g_ui_cursor_row = i;     g_0609029C(RMP(tbl + 0));
        g_ui_cursor_column = 12; g_ui_cursor_row = i + 2; g_0609029C(RMP(tbl + 4));
        g_ui_cursor_column = 12; g_ui_cursor_row = i + 4; g_0609029C(RMP(tbl + 8));
    }

    if (RU16(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = M8(ent, 0x43);
        M32(ent, 0x44) = 1;
        M32(ent, 0x48) = RS32(0x06051288 + TASK_S8(task, state) * 4);
        M32(ent, 0x5C) = 0;
        M32(ent, 0x58) = 0;
        M32(ent, 0x54) = 0;
        M32(ent, 0x50) = 0;
        M32(ent, 0x4C) = 0;
    } else if (RU16(0x06098794 + 6) & 0x0408) {
        s32 v = M32(ent, 0x40);
        M32(ent, 0x40) = (v < 7) ? v + 1 : 2;
    }

    g_ui_cursor_column = 12;
    g_ui_cursor_row = ((M32(ent, 0x40) - 2) * 2) + 6;
    g_0609029C((void *)(uintptr_t)0x0605142C);   /* tail-call glyph thunk */
}
/* fidelity: HIGH - hand-traced vs fn.py disassembly (0x2094c4..0x2095ac). */

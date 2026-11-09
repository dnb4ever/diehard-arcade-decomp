/* results_draw_state8_stats [0x0220a0b0]: State-8 five-row labels and resident field readout. */
#include "dh_work.h"
#include "dh.h"

/* 0x0220a0b0 - func_0220a0b0: results task state 8.
 * Split out of the former multi-function file 02209188.c (one function per
 * file); see 02209188.c for the family overview, the dispatcher, and the HUD
 * thunk conventions (g_060997A4/A8/AC cursor, *g_0609029C glyph, *g_060902A0
 * printf, *g_06000644 resident get-field). */

extern s32   g_ui_cursor_column;                                 /* cursor color/x */
extern s32   g_ui_cursor_row;                                 /* cursor mode/y  */
extern s32   g_060997AC;                                 /* cursor font    */
extern void (*g_0609029C)(void *desc);                   /* ui_glyph thunk */
extern void (*g_060902A0)(s32 fmt, ...);                 /* ui_printf thunk */
extern s32  (*g_06000644)(s32 op, void *addr, s32 z);    /* resident get-field fp */
extern s32   g_06098804;      /* work-buffer base pointer */
extern void results_draw_header(s32 code, void *ent);  /* 0x022093f8 results header */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))

/* ========================= state 8 (0x20a0b0) ========================= */
/* 5-row label block, event -> state 12, then a 7x3 numeric grid from the work
 * buffer (g_06098804 + row*0x10 + {0x90,0x94,0x98}) via the resident get-field fp. */
void results_draw_state8_stats(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32   i;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 0; i < 5; i++) {
        g_ui_cursor_column = 1; g_ui_cursor_row = i + 6;
        g_0609029C(RMP(0x060905F4 + i * 4));
    }

    if (RU16(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 12;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 7;
    for (i = 0; i < 7; i++) {
        /* each field: fetch (g_06098804 re-read per call), then position + print */
        s32 v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + i * 0x10 + 0x90), 0);
        g_ui_cursor_column = i * 4 + 0xF; g_ui_cursor_row = 8;   g_060902A0(0x06051434, v);
        v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + i * 0x10 + 0x94), 0);
        g_ui_cursor_column = i * 4 + 0xF; g_ui_cursor_row = 9;   g_060902A0(0x06051434, v);
        v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + i * 0x10 + 0x98), 0);
        g_ui_cursor_column = i * 4 + 0xF; g_ui_cursor_row = 0xA; g_060902A0(0x06051434, v);
    }
    g_060997AC = 1;
}
/* fidelity: HIGH - manually verified instruction-by-instruction vs the asm
 * (0x20a0b0..0x20a200): header loop, event -> state 12, and the 7x3 grid.  The
 * fetch/print calls interleave (get, print, get, print, ...) as in the ROM; the
 * printf thunk args are stack-passed (fmt, value).  difftest SKIPs it (indirect
 * thunk calls / raw RAM macros). */

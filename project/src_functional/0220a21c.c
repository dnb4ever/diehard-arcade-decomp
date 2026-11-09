/* results_draw_state9_stats [0x0220a21c]: State-9 fourteen-row labels and resident field readout. */
#include "dh_work.h"
#include "dh.h"

/* 0x0220a21c - func_0220a21c: results task state 9.
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
#define RS32(a) (*(s32  *)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))

/* ========================= state 9 (0x20a21c) ========================= */
/* 14-row label block, event -> state 10, then a grouped numeric grid: 5 groups,
 * each of g_0605121C[group] items, each item printing two fields from the work
 * buffer (base + g_06051230[itemIndex] + {0x100,0x104}) via the resident fp. */
void results_draw_state9_stats(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32   i;
    uintptr_t tbl;
    s32   group, item, running;

    results_draw_header(TASK_S8(task, state), ent);

    /* header: 14 glyph rows (mode 6..19), color 5, table 0x06090608, stride 4 */
    for (i = 6, tbl = 0x06090608; i < 20; i++, tbl += 4) {
        g_ui_cursor_column = 5; g_ui_cursor_row = i; g_0609029C(RMP(tbl));
    }

    if (RU16(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 10;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 7;
    running = 0;
    for (group = 0; group < 5; group++) {
        /* count is re-read each iteration and compared unsigned (cmp/hs) */
        for (item = 0; (u32)item < (u32)RS32(0x0605121C + group * 4); item++) {
            s32 v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 +
                        RS32(0x06051230 + running * 4) + 0x100), 0);
            g_ui_cursor_column = group * 7 + 5; g_ui_cursor_row = item * 2 + 8;
            g_060902A0(0x06051434, v);
            v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 +
                    RS32(0x06051230 + running * 4) + 0x104), 0);
            g_ui_cursor_column = group * 7 + 8; g_ui_cursor_row = item * 2 + 9;
            g_060902A0(0x06051434, v);
            running++;
        }
    }
    g_060997AC = 1;
}
/* fidelity: HIGH - manually verified instruction-by-instruction vs the asm
 * (0x20a21c..0x20a35a): header loop (rows 6..19), event -> state 10, and the
 * grouped grid.  The second field's column/row (group*7+8, item*2+9) is confirmed:
 * the ROM reads them through frame slots shifted +8 by the first printf's two
 * stack-pushed args.  Fetch/print calls interleave; g_06098804 and the
 * g_06051230 entry are re-read for each fetch; the per-group count compare is
 * unsigned.  difftest SKIPs it (indirect thunk calls / raw RAM macros). */

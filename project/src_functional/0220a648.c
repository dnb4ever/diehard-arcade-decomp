#include "dh_work.h"
#include "dh.h"
/* 0x0220a648 (hud_grid_screen_build) - sibling of hud_stats_screen_build (0x0220a390) /
 * hud_tally_screen_build (0x0220a4f0); rebuilds a 10-row x 3-column value grid each frame.
 * (Real entry; cart 0x20a612 is a pool-contaminated pre-entry of this one.)
 *
 * ABI: arg0 = task; ent = task->unk8. Steps:
 *  1. func_022093f8(task->unk10, ent) - shared HUD header/frame builder.
 *  2. Header glyphs: 16 rows (g_060997A8 = 6..21, color g_060997A4 = 11) from the ROM glyph
 *     table at 0x06090694 via *g_0609029C (2x-unrolled; re-rolled, 16 is even).
 *  3. If g_06098794.unk6 (u16 @ +6) has bits 0x0204 set: task->unk10 = 1, ent->unk40 = 5,
 *     ent->unk44 = 1.
 *  4. Grid: 10 groups (row = group+10), each printing 3 fields in columns 17/23/29. Field j
 *     of group g is read from the work buffer (*g_06098804 + g*12 + 0x280 + j*4) via the
 *     resident get-field fp *g_06000644 and printed with *g_060902A0(fmt 0x06051434, value).
 *     Then one final field at *g_06098804 + 0x2f8 is printed at column 29 / row 21.
 *     g_060997AC gates the block (7 during, 1 after).
 *
 * All *g_060902A0 sites pass (fmt, value) - 2 stack args, verified from the push sequence
 * (each inner call cleaned together by add #24,r15), modeled per g_060902A0(fmt, ...).
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20a648..0x20a7a6). The inner loop runs
 * exactly one 3-wide unrolled pass (r9 0->3). g_06000644 returns a u16 field (extu.w). */

extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern s32 g_060997AC;
extern void (*g_0609029C)(void *rec);                  /* ui_glyph thunk          */
extern void (*g_060902A0)(s32, ...);                   /* ui_printf thunk         */
extern s32  (*g_06000644)(s32 op, void *addr, s32 z);  /* resident get-field fp   */
extern s32   g_06098804;                                /* work-buffer base pointer */

extern void results_draw_header(s32 code, void *ent);          /* 0x2093f8 */

void hud_grid_screen_build(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32 i, outer, j;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 0; i < 16; i++) {
        g_ui_cursor_column = 11;
        g_ui_cursor_row = 6 + i;
        g_0609029C(*(void **)(uintptr_t)(0x06090694 + i * 4));
    }

    if (*(u16 *)(uintptr_t)(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 1;
        M32(ent, 0x40) = 5;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 7;
    for (outer = 0; outer < 10; outer++) {
        for (j = 0; j < 3; j++) {
            s32 val = (u16)g_06000644(
                0, (void *)(uintptr_t)(g_06098804 + outer * 12 + 0x280 + j * 4), 0);
            g_ui_cursor_column = 17 + j * 6;
            g_ui_cursor_row = outer + 10;
            g_060902A0(0x06051434, val);
        }
    }
    {
        s32 val = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x2f8), 0);
        g_ui_cursor_column = 29;
        g_ui_cursor_row = 21;
        g_060902A0(0x06051434, val);
    }
    g_060997AC = 1;
}

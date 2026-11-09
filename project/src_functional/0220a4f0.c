#include "dh_work.h"
#include "dh.h"
/* 0x0220a4f0 (hud_tally_screen_build) - sibling of hud_stats_screen_build (0x0220a390);
 * rebuilds a two-column tally HUD each frame.
 *
 * ABI: arg0 = task; ent = task->unk8. Steps:
 *  1. func_022093f8(task->unk10, ent) - shared HUD header/frame builder.
 *  2. Header glyphs: 14 rows (g_060997A8 = 6..19, color g_060997A4 = 5) from the ROM glyph
 *     table at 0x0609065c via *g_0609029C (2x-unrolled in asm; re-rolled, 14 is even).
 *  3. If g_06098794.unk6 (u16 @ +6) has bits 0x0204 set: task->unk10 = 12, ent->unk44 = 1.
 *  4. Tally block: 5 groups (counts at 0x0605121c[0..4]), rows by 7 per group. Each item
 *     reads one field (value) from the work buffer (*g_06098804 + table 0x06051230[running
 *     index] + 0x108) via *g_06000644 and prints it, then prints a 0 in the adjacent cell:
 *       col rowbase+5 / row item*2+8 : the value;
 *       col rowbase+8 / row item*2+9 : 0.
 *     Both via *g_060902A0(fmt 0x06051434, v). g_060997AC gates (7 during, 1 after).
 *
 * Both *g_060902A0 sites pass (fmt, value) - 2 stack args each, verified from the push
 * sequence + add #16,r15 (4 longs = 2 calls), modeled per the g_060902A0(fmt, ...) convention.
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20a4f0..0x20a5fe). g_06000644 returns a
 * u16 field (extu.w). No divide here (unlike 0x20a390). */

extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern s32 g_060997AC;
extern void (*g_0609029C)(void *rec);                  /* ui_glyph thunk          */
extern void (*g_060902A0)(s32, ...);                   /* ui_printf thunk         */
extern s32  (*g_06000644)(s32 op, void *addr, s32 z);  /* resident get-field fp   */
extern s32   g_06098804;                                /* work-buffer base pointer */

extern void results_draw_header(s32 code, void *ent);          /* 0x2093f8 */

void hud_tally_screen_build(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32 i, outer, inner, idx = 0;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 0; i < 14; i++) {
        g_ui_cursor_column = 5;
        g_ui_cursor_row = 6 + i;
        g_0609029C(*(void **)(uintptr_t)(0x0609065c + i * 4));
    }

    if (*(u16 *)(uintptr_t)(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 12;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 7;
    for (outer = 0; outer < 5; outer++) {
        s32 count = *(s32 *)(uintptr_t)(0x0605121c + outer * 4);
        for (inner = 0; inner < count; inner++) {
            s32 rec = g_06098804 + *(s32 *)(uintptr_t)(0x06051230 + idx * 4);
            s32 val = (u16)g_06000644(0, (void *)(uintptr_t)(rec + 0x108), 0);
            g_ui_cursor_column = outer * 7 + 5;
            g_ui_cursor_row = inner * 2 + 8;
            g_060902A0(0x06051434, val);
            g_ui_cursor_column = outer * 7 + 8;
            g_ui_cursor_row = inner * 2 + 9;
            g_060902A0(0x06051434, 0);
            idx++;
        }
    }
    g_060997AC = 1;
}

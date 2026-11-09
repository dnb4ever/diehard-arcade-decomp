#include "dh_work.h"
#include "dh.h"
/* 0x0220a390 (hud_stats_screen_build) - rebuild the per-player stats/results HUD each frame.
 *
 * ABI: arg0 = task; ent = task->unk8. Steps:
 *  1. func_022093f8(task->unk10, ent) - the shared HUD header/frame builder.
 *  2. Header glyph rows: 7 rows (g_060997A8 = 6,8,..,18; color g_060997A4 = 5) each drawing
 *     a glyph record from the ROM table at 0x06090640 via the ui_glyph thunk *g_0609029C.
 *  3. If g_06098794.unk6 (u16 @ +6) has bits 0x0204 set (a confirm/advance button), arm the
 *     advance: task->unk10 = 11, ent->unk44 = 1.
 *  4. Stats block: 5 groups (0x0605121c[0..4] give each group's item count), rows stepping
 *     by 7 per group (g_060997A4), columns by 2 per item (g_060997A8 = item*2 + 8). For each
 *     item it reads a current/max pair from the work buffer (*g_06098804 + table 0x06051230
 *     [running index] + 0x100/+0x104) via the resident get-field fp *g_06000644, computes a
 *     0..100 percentage = (100 * hw_divide(cur,max)) >> 16 (0 when max==0), and prints it
 *     with the ui_printf thunk *g_060902A0(fmt 0x06051434, percent). g_060997AC gates the
 *     print (7 during the block, 1 after).
 *
 * The *g_060902A0 call is the known variadic ui_printf thunk; its one site here passes
 * (fmt, percent) - read directly from the push sequence + add #8,r15 (2 stack args, fmt
 * lowest = first), modeled per the project's established g_060902A0(fmt, ...) convention.
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20a390..0x20a49e). Sole bsrf -> 0x213234
 * (sh2_hw_divide, r4=cur r5=max). g_06000644 returns a u16 field value (extu.w). */

extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern s32 g_060997AC;
extern void (*g_0609029C)(void *rec);          /* ui_glyph thunk          */
extern void (*g_060902A0)(s32, ...);           /* ui_printf thunk         */
extern s32  (*g_06000644)(s32 op, void *addr, s32 z);  /* resident get-field fp */
extern s32   g_06098804;                        /* work-buffer base pointer */

extern void results_draw_header(s32 code, void *ent);  /* 0x2093f8 */
extern s32  sh2_hw_divide(s32 num, s32 den);   /* 0x213234 sh2_hw_divide  */

void hud_stats_screen_build(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32 i, outer, inner, idx = 0;

    results_draw_header(TASK_S8(task, state), ent);

    for (i = 0; i < 7; i++) {
        g_ui_cursor_column = 5;
        g_ui_cursor_row = 6 + i * 2;
        g_0609029C(*(void **)(uintptr_t)(0x06090640 + i * 4));
    }

    if (*(u16 *)(uintptr_t)(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 11;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 7;
    for (outer = 0; outer < 5; outer++) {
        s32 count = *(s32 *)(uintptr_t)(0x0605121c + outer * 4);
        for (inner = 0; inner < count; inner++) {
            s32 rec = g_06098804 + *(s32 *)(uintptr_t)(0x06051230 + idx * 4);
            s32 cur = (u16)g_06000644(0, (void *)(uintptr_t)(rec + 0x100), 0);
            s32 max = (u16)g_06000644(0, (void *)(uintptr_t)(rec + 0x104), 0);
            s32 pct = (max != 0)
                ? (s16)((u32)(100 * sh2_hw_divide(cur, max)) >> 16)
                : 0;
            g_ui_cursor_column = 7 * (outer + 1);
            g_ui_cursor_row = inner * 2 + 8;
            g_060902A0(0x06051434, pct);
            idx++;
        }
    }
    g_060997AC = 1;
}

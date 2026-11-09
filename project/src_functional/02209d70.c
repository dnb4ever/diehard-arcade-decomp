/* results_draw_final_stats [0x02209d70]: State-5 resident counter queries and numeric formatting. */
#include "dh_work.h"
#include "dh.h"

/* 0x02209d70 - func_02209d70: results task state 5 (final stats readout).
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
extern u32  (*g_06000638)(s32 op, s32 v);                /* BIOS value service (state 5); v widened to s32 to carry full sdiv quotient */
extern s32   g_06098804;      /* work-buffer base pointer */
extern s32  shc_signed_divide_raw_entry(s32 dividend, s32 divisor); /* 0x224a06c sdiv32 (signed; r1=dividend/r0=divisor ABI) */
extern void results_draw_header(s32 code, void *ent);  /* 0x022093f8 results header */

/* raw-RAM access helpers */
#define RMP(a)  (*(void **)(uintptr_t)(a))
#define RU16(a) (*(u16  *)(uintptr_t)(a))

/* emit a 32-bit total as four byte columns (x=0x16/0x1A/0x1E/0x22, bytes
 * >>24/>>16/>>8/&0xFF) on `row`, via the ui_printf thunk with fmt 0x0605144C. */
static void hud_draw_u32_bytes(s32 row, u32 v)
{
    s32 col;
    for (col = 0; col < 4; col++) {
        g_ui_cursor_column = 0x16 + col * 4;
        g_ui_cursor_row = row;
        g_060902A0(0x0605144C, (v >> (24 - col * 8)) & 0xFF);
    }
}

/* ========================= state 5 (0x209d70) ========================= */
/* Final stats readout screen.  15-row label block, event -> state 8, then a large
 * block of per-stat numeric readouts from the work buffer (resident fp / BIOS fp /
 * sdiv32), printed as byte columns. */
void results_draw_final_stats(void *task)
{
    void *ent = TASK_PTR(task, work);
    s32   i;
    uintptr_t tbl;

    results_draw_header(TASK_S8(task, state), ent);

    /* header: 15 glyph rows (mode 6..20), color 6, table 0x060905B8, stride 4 */
    for (i = 6, tbl = 0x060905B8; i < 21; i++, tbl += 4) {
        g_ui_cursor_column = 6; g_ui_cursor_row = i; g_0609029C(RMP(tbl));
    }

    if (RU16(0x06098794 + 6) & 0x0204) {
        TASK_S8(task, state) = 8;
        M32(ent, 0x44) = 1;
    }

    g_060997AC = 7;
    /* stats-readout body (0x209dec..0x20a098).  Six per-column u16 fields from the
     * work buffer (g_06098804 + {0x70,0x74,0x78,0x80,0x84,0x88}, resident op 0),
     * printed with fmt 0x06051448 in column x=33; then three 32-bit totals at
     * +0x58/+0x60/+0x68 (resident op 1 -> *g_06000638), each shown as four byte
     * columns with fmt 0x0605144C.  The 0x78 and 0x88 fields are also summed into a
     * shared divisor; total 3 is num(0x68)/divisor via sdiv32 when the divisor is
     * non-zero (else 0).  No 64-bit macl/mach math is present - the earlier note was
     * a precaution; the "stack-shift" was the compiler aliasing one local slot for
     * the 0x78 save and the 0x88 accumulate (both land at the ent-save slot, which
     * is dead after the event block above).  Each field is fetched before the
     * cursor stores of its row, and g_06098804 is re-read for every fetch, as in ROM. */
    {
        s32 v, f2, f5, denom, num;
        u32 t1, t2, t3;

        v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x70), 0);
        g_ui_cursor_column = 33; g_ui_cursor_row = 7;
        g_060902A0(0x06051448, v);
        v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x74), 0);
        g_ui_cursor_column = 33; g_ui_cursor_row = 8;
        g_060902A0(0x06051448, v);
        f2 = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x78), 0);
        g_ui_cursor_column = 33; g_ui_cursor_row = 9;
        g_060902A0(0x06051448, f2);
        v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x80), 0);
        g_ui_cursor_column = 33; g_ui_cursor_row = 12;
        g_060902A0(0x06051448, v);
        v = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x84), 0);
        g_ui_cursor_column = 33; g_ui_cursor_row = 13;
        g_060902A0(0x06051448, v);
        f5 = (u16)g_06000644(0, (void *)(uintptr_t)(g_06098804 + 0x88), 0);
        denom = f2 + f5;                       /* shared divisor for total 3 */
        g_ui_cursor_column = 33; g_ui_cursor_row = 14;
        g_060902A0(0x06051448, f5);

        t1 = g_06000638(1, g_06000644(1, (void *)(uintptr_t)(g_06098804 + 0x58), 0));
        hud_draw_u32_bytes(16, t1);
        t2 = g_06000638(1, g_06000644(1, (void *)(uintptr_t)(g_06098804 + 0x60), 0));
        hud_draw_u32_bytes(18, t2);

        num = g_06000644(1, (void *)(uintptr_t)(g_06098804 + 0x68), 0);
        t3  = (denom != 0) ? g_06000638(1, shc_signed_divide_raw_entry(num, denom)) : 0;
        hud_draw_u32_bytes(20, t3);
    }

    g_060997AC = 1;
}
/* fidelity: HIGH - header loop, event transition, and the stats-readout body (six
 * u16 fields, three 32-bit totals incl. the sdiv32 average) hand-traced vs fn.py
 * disassembly (0x209d70..0x20a0b0); ui_printf args modelled (fmt, value), and the
 * *g_06000638 v-arg widened to s32 to carry the full sdiv quotient. */

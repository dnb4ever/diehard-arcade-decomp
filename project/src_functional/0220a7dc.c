#include "dh_work.h"
#include "dh.h"
/* 0x0220a7dc (continue_prompt_handler) - task step for the "continue?" prompt: draws the
 * header + menu glyphs, runs the countdown, and services the yes/no cursor. task->unk8 is
 * the prompt entity.
 *
 *   func_022093f8((s8)task->unk10, ent);                 // shared 3-glyph header/status block
 *   // menu rows (column 0xD): two glyph records per table pair, rows 6 and 8
 *   for (tbl = g_060906D4, row = 6; row < 0xA; tbl += 2) {
 *       g_060997A4 = 0xD; g_060997A8 = row;     (*g_0609029C)(tbl[0]);  row += 2;
 *       g_060997A4 = 0xD; g_060997A8 = row;     (*g_0609029C)(tbl[1]);  row += 2;
 *   }
 *   if (ent->unk4C <= 0) {                               // timer expired -> interactive
 *       pad = g_06098794.u16@6;
 *       if (pad & 0x204) {                               // confirm
 *           switch (ent->unk48) {                        // cursor: 0 = continue, 1 = quit
 *               case 0: for (i=0;i<0xF00;i+=8) (*g_06000644)(3, g_06098804 + i, 0);
 *                       ent->unk4C = 0x2D; break;         // re-arm timer
 *               case 1: task->unk10 = 1; ent->unk40 = 6; ent->unk44 = 1; break;  // leave
 *           }
 *       } else if (pad & 0x408) {                         // move cursor (toggle 0<->1)
 *           ent->unk48 = (ent->unk48 >= 1) ? 0 : ent->unk48 + 1;
 *       }
 *       g_060997A4 = 0xD; g_060997A8 = ent->unk48 * 2 + 6;
 *       (*g_0609029C)(0x0605142C);                        // cursor marker
 *       return;
 *   }
 *   ent->unk4C--;                                         // still counting down
 *   g_060997AC = 2; g_060997A4 = 0x11; g_060997A8 = 0x14;
 *   (*g_0609029C)(0x06051450);                            // countdown numerals
 *   g_060997AC = 1;
 *   if (ent->unk4C == 0) { task->unk10 = 1; ent->unk40 = 6; ent->unk44 = 1; }
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20a7dc..0x20a94e). Sole bsrf resolves to
 * func_022093f8 (0x2093f8), called with r4=(s8)task->unk10 (bsrf delay slot) and r5=ent.
 * *g_0609029C is void(*)(void*); the cursor color/mode ride in g_060997A4/A8. The case-0
 * resident-command fill (fp *g_06000644) is 3x unrolled in ROM; re-rolled here. */

extern void   results_draw_header(s32 code, void *ent);          /* 0x2093f8 */
extern void (*g_0609029C)(void *rec);                     /* UI glyph thunk */
extern void (*g_06000644)(s32 op, void *addr, s32 c);     /* resident command fp */
extern void  *g_060906D4[];                               /* menu glyph-record table */
extern void  *g_06098804;                                 /* work-buffer base pointer */
extern s8     g_06098794;                                 /* syspad; u16@6 = buttons */
extern s32    g_ui_cursor_column;                                 /* HUD cursor column */
extern s32    g_ui_cursor_row;                                 /* HUD cursor row / mode */
extern s32    g_060997AC;                                 /* HUD cursor flag */

void continue_prompt_handler(void *task) {
    void  *ent = TASK_PTR(task, work);
    void **tbl;
    s32    row;

    results_draw_header((s8)TASK_S8(task, state), ent);

    for (tbl = g_060906D4, row = 6; row < 0xA; tbl += 2) {
        g_ui_cursor_column = 0xD; g_ui_cursor_row = row;     (*g_0609029C)(tbl[0]);
        row += 2;
        g_ui_cursor_column = 0xD; g_ui_cursor_row = row;     (*g_0609029C)(tbl[1]);
        row += 2;
    }

    if (M32(ent, 0x4C) <= 0) {
        u16 pad = MU16(&g_06098794, 6);
        if (pad & 0x204) {
            switch (M32(ent, 0x48)) {
            case 0: {
                s32 i;
                for (i = 0; i < 0xF00; i += 8)
                    (*g_06000644)(3, (char *)g_06098804 + i, 0);
                M32(ent, 0x4C) = 0x2D;
                break;
            }
            case 1:
                TASK_S8(task, state)  = 1;
                M32(ent, 0x40) = 6;
                M32(ent, 0x44) = 1;
                break;
            }
        } else if (pad & 0x408) {
            M32(ent, 0x48) = (M32(ent, 0x48) >= 1) ? 0 : M32(ent, 0x48) + 1;
        }
        g_ui_cursor_column = 0xD;
        g_ui_cursor_row = M32(ent, 0x48) * 2 + 6;
        (*g_0609029C)((void *)0x0605142C);
        return;
    }

    M32(ent, 0x4C) -= 1;
    g_060997AC = 2;
    g_ui_cursor_column = 0x11;
    g_ui_cursor_row = 0x14;
    (*g_0609029C)((void *)0x06051450);
    g_060997AC = 1;
    if (M32(ent, 0x4C) == 0) {
        TASK_S8(task, state)  = 1;
        M32(ent, 0x40) = 6;
        M32(ent, 0x44) = 1;
    }
}

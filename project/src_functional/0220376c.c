#include "dh_work.h"
#include "dh.h"
/* 0x0220376c (hud_status_service_2p) - rebuild both players' status HUD each frame. The
 * two halves draw at column 0 (P1) and column 0x16 (P2) via the UI glyph thunk
 * *g_0609029C; the text cursor color/mode is poked into g_060997A4 (column) and
 * g_060997A8 (row/mode) before every dispatch. task->unk8 holds the controlling record.
 *
 *   rec = task->unk8;  (*g_06000660)(0x060987D8);   // refresh working copy
 *   P1 (periodic): if player enabled (!(g_060987BC & rec->unk50)) { rec->unk40++;
 *       if (rec->unk40 odd) status(col 0, &g_060987D8, &g_060987D4) else blank(col 0) }
 *   P1 (event):    if (g_06098794.u16@6 & 1) status(col 0, &g_060987D8, &g_060987D4)
 *   P2 (periodic): if player enabled { if (rec->unk40 odd) status(col 0x16, &g_060987DC,
 *                                        &g_060987D5) else blank(col 0x16) }
 *   P2 (event):    if (g_06098794.u16@6 & 2) status(col 0x16, &g_060987DC, &g_060987D5)
 *
 * status(col, p, readyflag):
 *   emit the player glyph g_060902CC[(u8)p[3]] (mode 9); then a continue/warn glyph
 *   (mode 0xA, record 0x0609D7B0) when the active nibble of the shared byte g_060987E9
 *   exceeds 1 - low nibble & builder tag 7 when *readyflag==0, else high nibble & tag 8;
 *   then the lives glyph (mode 0xB): tag 5 if (u8)p[0] <= 1 else tag 6.
 * blank(col): emit g_060902F0 three times (modes 9/0xA/0xB) when the player is idle.
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20376c..0x203a44); zero computed
 * branches. *g_0609029C is void(*)(void*) (single reg arg r4; the cursor row travels in
 * the g_060997A8 global, not an argument). The warn-nibble byte g_060987E9 (=*0x060987D8
 * + 0x11) is shared by both halves. Record-builder helpers func_02203de8/203e0a/203e5e and
 * the refresh thunk *g_06000660 are out of this TU. The four draw blocks are identical in
 * the ROM apart from column/record/flag; factored into helpers here. */

extern s32  g_frame_tick;                 /* per-player HUD disable mask */
extern s8   g_06098794;                 /* syspad/status struct; u16@6 = HUD event bits  */
extern s8   g_060987D8;                 /* P1 working record (nibble byte at +0x11)       */
extern s8   g_060987DC;                 /* P2 working record                              */
extern s8   g_060987D4;                 /* P1 ready flag                                   */
extern s8   g_060987D5;                 /* P2 ready flag (= 0x060987D4 + 1)                */
extern u8   g_060987E9;                 /* shared continue/warn count (two nibbles)        */
extern s32  g_ui_cursor_column;                 /* HUD cursor column                               */
extern s32  g_ui_cursor_row;                 /* HUD cursor row / mode                           */
extern void *g_060902CC;                /* player glyph-record table                       */
extern void *g_060902F0;                /* idle/blank glyph record                         */
extern void (*g_0609029C)(void *rec);   /* UI glyph dispatch thunk                         */
extern void (*g_06000660)(void *p);     /* working-copy refresh thunk                      */

extern void hud_copy_status_template(s32 tag);               /* 0x203de8 build glyph record 0x0609D7B0 */
extern void hud_encode_status_item(s32 tag, void *p);       /* 0x203e0a build lives record            */
extern void hud_encode_config_nibble(s32 a, s32 b);           /* 0x203e5e warn/continue helper          */

static void hud_status(s32 col, void *p, s8 *readyflag) {
    g_ui_cursor_column = col;
    g_ui_cursor_row = 9;
    (*g_0609029C)(MP(&g_060902CC, (u32)MU8(p, 3) * 4));

    if (*readyflag == 0) {
        if (((u32)g_060987E9 & 0xF) > 1) {
            hud_copy_status_template(7);
            hud_encode_config_nibble(1, 1);
            g_ui_cursor_column = col;
            g_ui_cursor_row = 0xA;
            (*g_0609029C)((void *)0x0609D7B0);
        }
    } else if ((s32)((u32)g_060987E9 >> 4) > 1) {
        hud_copy_status_template(8);
        hud_encode_config_nibble(0, 0);
        g_ui_cursor_column = col;
        g_ui_cursor_row = 0xA;
        (*g_0609029C)((void *)0x0609D7B0);
    }

    hud_copy_status_template(((s32)(u32)MU8(p, 0) <= 1) ? 5 : 6);
    hud_encode_status_item(0xE, p);
    g_ui_cursor_column = col;
    g_ui_cursor_row = 0xB;
    (*g_0609029C)((void *)0x0609D7B0);
}

static void hud_blank(s32 col) {
    g_ui_cursor_column = col; g_ui_cursor_row = 9;   (*g_0609029C)(g_060902F0);
    g_ui_cursor_column = col; g_ui_cursor_row = 0xA; (*g_0609029C)(g_060902F0);
    g_ui_cursor_column = col; g_ui_cursor_row = 0xB; (*g_0609029C)(g_060902F0);
}

void hud_status_service_2p(void *task) {
    void *rec = TASK_PTR(task, work);

    (*g_06000660)(&g_060987D8);

    if (!(g_frame_tick & M32(rec, 0x50))) {
        M32(rec, 0x40) += 1;
        if (M32(rec, 0x40) & 1)
            hud_status(0, &g_060987D8, &g_060987D4);
        else
            hud_blank(0);
    }
    if (MU16(&g_06098794, 6) & 1)
        hud_status(0, &g_060987D8, &g_060987D4);

    if (!(g_frame_tick & M32(rec, 0x50))) {
        if (M32(rec, 0x40) & 1)
            hud_status(0x16, &g_060987DC, &g_060987D5);
        else
            hud_blank(0x16);
    }
    if (MU16(&g_06098794, 6) & 2)
        hud_status(0x16, &g_060987DC, &g_060987D5);
}

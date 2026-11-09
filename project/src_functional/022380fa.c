/* hud_draw_indexed_label [0x022380fa]: Bounds index below 32, sets cursor/font and draws indexed label before restoring font. */
#include "dh.h"
/* 0x022380fa (callC) - if (u32)arg3 >= 32, return. Else store globals
 * (0x060997AC = arg0 & 15; 0x060997A4 = arg1; 0x060997A8 = arg2), then call the
 * function pointer at 0x0609029C with (0x0608C324 + 4*arg3); finally 0x060997AC = 1.
 * fidelity: HIGH - all 32-bit; arg3 compared unsigned (cmp/hs #32); table stride 4. */
extern s32 g_060997AC;
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern s32 g_0608C324;
extern void (*g_0609029C)(void *);
void hud_draw_indexed_label(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    if ((u32)arg3 >= 32)
        return;
    g_060997AC = arg0 & 15;
    g_ui_cursor_column = arg1;
    g_ui_cursor_row = arg2;
    g_0609029C((void *)(&g_0608C324 + arg3));
    g_060997AC = 1;
}

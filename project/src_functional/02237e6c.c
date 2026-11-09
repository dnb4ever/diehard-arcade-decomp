/* hud_draw_meter_label [0x02237e6c]: Draws the short or long meter label at the slot-selected cursor. */
#include "dh.h"
/* 0x02237e6c (callC) - sibling of func_02237e0e: set draw params from obj+2 then
 * dispatch via the function pointer g_0609029C with one of two table pointers.
 * w = (s16)obj[+2]: g_060997A4 = (w & 3)*10 + 2; g_060997A8 = ((w>>2)<<1) + 1.
 * if ((s16)obj[0] == 1) g_0609029C(&g_0608C294); else g_0609029C(&g_0608C2C4).
 * fidelity: HIGH - w 16-bit; >>2 arithmetic; globals 32-bit. */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern s8  g_0608C294;
extern s8  g_0608C2C4;
extern void (*g_0609029C)(void *);
void hud_draw_meter_label(void *obj) {
    s16 w = M16(obj, 2);
    g_ui_cursor_column = (w & 3) * 10 + 2;
    g_ui_cursor_row = ((s16)M16(obj, 2) >> 2) * 2 + 1;
    if (M16(obj, 0) == 1)
        g_0609029C((void *)&g_0608C294);
    else
        g_0609029C((void *)&g_0608C2C4);
}

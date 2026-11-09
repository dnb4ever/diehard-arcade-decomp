/* hud_draw_meter_icon [0x02237e0e]: Selects colour and cursor from the meter slot before drawing its icon. */
#include "dh.h"
/* 0x02237e0e (callC) - set up global draw params then dispatch via fn pointer 0x0609029C.
 * w = *(s16)(arg0+2): 0x060997A4 = (w & 3)*10 + 3; 0x060997A8 = ((w>>2)<<1)+1.
 * If (*(s16)(arg0+6) & 1): 0x060997AC = 15; else 0x060997AC = 0x0608C45C[ *(s8)(arg0+5) ] & 15.
 * Call (*0x0609029C)(0x0608C43C + 4*arg1); then 0x060997AC = 1.
 * fidelity: HIGH - w 16-bit; table byte (extu.b); arith shifts; globals 32-bit. */
extern s32 g_ui_cursor_column;
extern s32 g_ui_cursor_row;
extern s32 g_060997AC;
extern s8  g_0608C45C;
extern s8  g_0608C43C;
extern void (*g_0609029C)(void *);
void hud_draw_meter_icon(void *arg0, s32 arg1) {
    s16 w = M16(arg0, 2);
    g_ui_cursor_column = (w & 3) * 10 + 3;
    g_ui_cursor_row = ((M16(arg0, 2) >> 2) << 1) + 1;
    if (M16(arg0, 6) & 1)
        g_060997AC = 15;
    else
        g_060997AC = (&g_0608C45C)[ M8(arg0, 5) ] & 15;
    g_0609029C((void *)(&g_0608C43C + 4 * arg1));
    g_060997AC = 1;
}

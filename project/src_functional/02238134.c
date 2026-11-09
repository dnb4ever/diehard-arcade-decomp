/* hud_draw_status_panel [0x02238134]: Draws or clears the fixed status panel rectangle. */
#include "dh.h"
/* 0x02238134 (callC) - selects between two sprite/draw calls based on arg0.
 * arg0 != 0: func_02216858(0x7CDE, &g_060997B0, (void*)0x2241AFA4, 10, 6, 0x203).
 * arg0 == 0: func_02216a26(0x0C000C00, &g_060997B0, 10, 6, 25, 3, 0x203).
 * bsrf targets -> RAM 0x0601a858 / 0x0601aa26.
 * fidelity: MED - stack args ordered arg5..argN (lowest addr first); 0x2241AFA4 kept as absolute ptr. */
extern s32 g_060997B0;
extern void tilemap_copy_wrap_narrow(s32, void *, void *, s32, s32, s32);
extern void tilemap_fill_narrow(s32, void *, s32, s32, s32, s32, s32);
void hud_draw_status_panel(s32 arg0) {
    if (arg0 != 0)
        tilemap_copy_wrap_narrow(0x7CDE, &g_060997B0, (void *)0x2241AFA4, 10, 6, 0x203);
    else
        tilemap_fill_narrow(0x0C000C00, &g_060997B0, 10, 6, 25, 3, 0x203);
}

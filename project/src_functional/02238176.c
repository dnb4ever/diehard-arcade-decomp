/* hud_draw_status_digit [0x02238176]: Draws a selected status digit or clears its tile rectangle for an out-of-range value. */
#include "dh.h"
/* 0x02238176 (callC) - two sprite/draw calls selected by (u32)arg1.
 * arg1 <= 9: func_02216858((arg0<<12)|0xCDE, &g_060997B0, (void*)(0x2241AF54+8*arg1), 31, 6, 0x203).
 * arg1 >  9: func_02216a26(0x0C000C00, &g_060997B0, 31, 6, 3, 3, 0x203).
 * bsrf -> RAM 0x0601a858 / 0x0601aa26.
 * fidelity: MED - arg1 compared unsigned (cmp/hi #9); stack args lowest-addr first. */
extern s32 g_060997B0;
extern void tilemap_copy_wrap_narrow(s32, void *, void *, s32, s32, s32);
extern void tilemap_fill_narrow(s32, void *, s32, s32, s32, s32, s32);
void hud_draw_status_digit(s32 arg0, s32 arg1) {
    if ((u32)arg1 <= 9)
        tilemap_copy_wrap_narrow((arg0 << 12) | 0xCDE, &g_060997B0,
                      (void *)(uintptr_t)(0x2241AF54 + 8 * arg1), 31, 6, 0x203);
    else
        tilemap_fill_narrow(0x0C000C00, &g_060997B0, 31, 6, 3, 3, 0x203);
}

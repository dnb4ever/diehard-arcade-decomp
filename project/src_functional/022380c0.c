/* hud_draw_counter_icon [0x022380c0]: Sets text colour/cursor and invokes the selected counter icon descriptor. */
#include "dh.h"
/* 0x022380c0 HUD draw primitive. If mode (arg3) < 5 (unsigned): stash params into the
 * draw scratch block, dispatch the glyph/draw routine via function pointer at 0x0609029c
 * with &g_0608c3a4[mode], then mark the block ready.
 *   0x060997ac = arg0 & 0xf;  0x060997a4 = arg1;  0x060997a8 = arg2;
 *   (*(fp @0x0609029c))(&g_0608c3a4[arg3]);  0x060997ac = 1;
 * fidelity: HIGH - cmp/hs unsigned bound; arg3 spilled/reloaded for the index. */
#define G32(a) (*(s32 *)(uintptr_t)(a))

void hud_draw_counter_icon(s32 arg0, s32 arg1, s32 arg2, u32 arg3) {
    if (arg3 < 5U) {
        G32(0x060997ac) = arg0 & 0xf;
        G32(DH_UI_CURSOR_COLUMN_ADDR) = arg1;
        G32(DH_UI_CURSOR_ROW_ADDR) = arg2;
        (*(void (**)(void *))(uintptr_t)0x0609029c)(
            (void *)((uintptr_t)0x0608c3a4 + arg3 * 4));
        G32(0x060997ac) = 1;
    }
}

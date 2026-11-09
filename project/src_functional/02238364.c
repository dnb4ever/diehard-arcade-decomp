/* hud_draw_level_record [0x02238364]: Per-slot threshold selects fallback label or descriptor-driven drawing. */
#include "dh.h"
/* 0x02238364 - dispatch handling for table slot `arg0` (0..7).
 * arg0 = slot index (ignored when >= 8); arg1 = threshold/level; arg2 = payload.
 * record = &g_0608c484[arg0 * 16].
 * If arg1 >= record->unkC: store 0 at g_060997a4, arg2 at g_060997a8, then
 *   tail-call (*g_0609029c)(g_0608c508).
 * Else: p = (u8 *)record->unk8 + arg1 * 8;
 *   func_02216858(0xECDD, g_060997b0, p, (44 - (s8)*p) / 2, arg2, 0x203).
 * bsrf resolved: 0x216858. fidelity: HIGH (modelled void; the >=unkC branch is a
 * tail-call whose result, if any, is discarded on the other paths). */
extern u8   g_0608c484[];
extern s32  g_ui_cursor_column_legacy;
extern s32  g_ui_cursor_row_legacy;
extern u8   g_0608c508[];
extern u8   g_060997b0[];
extern void (*g_0609029c)(void *);
extern void tilemap_copy_wrap_narrow(s32 a, void *b, void *p, s32 n, s32 d, s32 e);

void hud_draw_level_record(u32 arg0, u32 arg1, s32 arg2) {
    void *record;
    u8 *p;

    if (arg0 >= 8U)
        return;
    record = &g_0608c484[arg0 * 16];
    if (arg1 >= MU32(record, 0xC)) {
        g_ui_cursor_column_legacy = 0;
        g_ui_cursor_row_legacy = arg2;
        g_0609029c(g_0608c508);
        return;
    }
    p = (u8 *)MP(record, 8) + arg1 * 8;
    tilemap_copy_wrap_narrow(0xECDD, g_060997b0, p, (44 - (s8)*p) / 2, arg2, 0x203);
}

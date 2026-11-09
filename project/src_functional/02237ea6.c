/* hud_tick_player_popups [0x02237ea6]: Two fixed player popup records dispatch glyph updates and countdown visibility. */
#include "dh.h"
/* 0x02237ea6 - update the two on-screen damage/score popup slots (g_0609860c
 * and g_06098614). arg0 = HUD state; dispatch on arg0->unk10.
 * case 0: zero both slots (unk0 word, unk4 long) and advance into case 1.
 * case 1: for each slot, flush a pending "show" (unk0 != 0) via two g_0609029c
 *   dispatch calls, then if the slot countdown unk4 > 0, decrement it, blit the
 *   current digit tile to VRAM via func_0221632e, and refresh the text buffer
 *   g_0609d7b0 through copy_longs + g_0609029c.
 * The per-slot work is identical apart from the sprite channel id, VRAM base and
 * the two tile-source templates, so it is factored into hud_popup().
 * g_060997a4/a8/ac are the shared dispatch parameter globals.
 * branches resolved: bsrf 0x21632e, 0x21326c; jsr via g_0609029c.
 * fidelity: HIGH. */
extern void (*g_0609029c)(void *arg);
extern s32  g_ui_cursor_column_legacy;
extern s32  g_ui_cursor_row_legacy;
extern s32  g_060997ac;
extern u32  g_0608c3b8[];
extern u8   g_0609d7b0[];
extern u8   g_0608c414[];
extern u8   g_0608c41c[];
extern u8   g_0608c424[];
extern u8   g_0608c42c[];
extern u8   g_0608c434[];
extern u8   g_0609860c[];
extern u8   g_06098614[];
extern void vdp2_queue_attribute(s32 a, u32 src, u32 dst, s32 n);
extern void copy_longs(void *src, void *dst, s32 n);

static void hud_popup(void *slot, s32 channel, u32 vram,
                      void *src_lt6, void *src_ge6) {
    s32 cnt;
    s32 idx;

    if (M16(slot, 0) != 0) {
        M16(slot, 0) = 0;
        g_ui_cursor_column_legacy = channel; g_ui_cursor_row_legacy = 12; g_0609029c(g_0608c414);
        g_ui_cursor_column_legacy = channel; g_ui_cursor_row_legacy = 13; g_0609029c(g_0608c414);
    }

    cnt = M32(slot, 4);
    if (cnt <= 0)
        return;
    M32(slot, 4) = cnt - 1;
    idx = 12 - cnt;

    vdp2_queue_attribute(13, g_0608c3b8[MU16(slot, 2)] + idx * 128, vram + idx * 128, 32);
    g_060997ac = 4;
    if ((u32)idx < 6U) {
        g_ui_cursor_column_legacy = channel; g_ui_cursor_row_legacy = 12;
        copy_longs(src_lt6, g_0609d7b0, 2);
        g_0609d7b0[idx + 2] = 0;
    } else {
        g_ui_cursor_column_legacy = channel; g_ui_cursor_row_legacy = 13;
        copy_longs(src_ge6, g_0609d7b0, 2);
        g_0609d7b0[idx - 4] = 0;
    }
    g_0609029c(g_0609d7b0);
    g_060997ac = 1;
}

void hud_tick_player_popups(void *arg0) {
    switch (M8(arg0, 0x10)) {
    case 0:
        M16(g_06098614, 0) = 0;
        M16(g_0609860c, 0) = 0;
        M32(g_06098614, 4) = 0;
        M32(g_0609860c, 4) = 0;
        M8(arg0, 0x10)++;
        /* fallthrough */
    case 1:
        hud_popup(g_0609860c, 1, 0x25E65F00u, g_0608c41c, g_0608c424);
        hud_popup(g_06098614, 37, 0x25E66500u, g_0608c42c, g_0608c434);
        break;
    default:
        break;
    }
}

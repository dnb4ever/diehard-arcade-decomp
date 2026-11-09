/* hud_draw_segmented_meter [0x02237be4]: Encodes full and partial meter tiles into the text buffer and dispatches the glyph list. */
#include "dh.h"
/* 0x02237be4 - render a segmented gauge/bar (health or similar) into the shared
 * text buffer g_0609d7b0 and submit it via the g_0609029c dispatch.
 * arg0->unk2 selects palette/position (writes dispatch params g_060997a4/a8/ac);
 * arg0->unk0 picks the bar size: mode 1 = short bar (value clamped 0..0xC0,
 * half-point 0x60, 4-wide header), else = long bar (0..0x200, half 0x100,
 * 8-wide header). The value is split into full 16-unit tiles plus a partial
 * tile; above the half-point a brighter tile set is used, below a dimmer one.
 * branches resolved: temp_r8 = copy_longs (0x02237c44 + 0xFFFDB628); the
 * terminal g_0609029c call takes only r4 = &g_0609d7b0[3] (r5/r6 are stale).
 * fidelity: HIGH. */
extern s32  g_ui_cursor_column_legacy;
extern s32  g_ui_cursor_row_legacy;
extern s32  g_060997ac;
extern u8   g_0609d7b0[];
extern u8   g_0608c2a4[];
extern u8   g_0608c2b4[];
extern u8   g_0608c2e4[];
extern u8   g_0608c304[];
extern void copy_longs(void *src, void *dst, s32 n);
extern void (*g_0609029c)(void *arg);

/* fill the bar body starting at g_0609d7b0[3]; val is the residual after the
 * half-point subtraction, 'above' selects the bright tile set. */
static void fill_bar(s32 val, int above) {
    s8 *p = (s8 *)&g_0609d7b0[3];
    s32 n = val >> 4;
    s32 i = 0;
    s8 full    = above ? (s8)-0x5D : (s8)-0x6F;
    s8 partial = above ? (s8)-0x6E : (s8)-0x80;
    s8 endcap  = above ? (s8)-0x5E : (s8)-0x70;

    for (; i < n; i++)
        *p++ = full;

    if (val & 0xF)
        *p = (s8)((val & 0xF) + partial);
    else if (i != 0)
        *(--p) = endcap;
}

void hud_draw_segmented_meter(void *arg0) {
    s32 v;

    g_ui_cursor_column_legacy = ((M16(arg0, 2) & 3) * 0xA) + 5;
    g_ui_cursor_row_legacy = ((M16(arg0, 2) >> 2) * 2) + 1;
    if (M16(arg0, 2) < 4) {
        v = M32(arg0, 8);
        g_060997ac = (v < 0x40 && v != 0) ? 6 : 5;
    } else {
        g_060997ac = 4;
    }

    if (M16(arg0, 0) == 1) {
        if (M32(arg0, 8) > 0xC0)
            M32(arg0, 8) = 0xC0;
        else if (M32(arg0, 8) < 0)
            M32(arg0, 8) = 0;
        v = M32(arg0, 8);
        if (v > 0x60) {
            copy_longs(g_0608c2b4, g_0609d7b0, 4);
            fill_bar(v - 0x60, 1);
        } else {
            copy_longs(g_0608c2a4, g_0609d7b0, 4);
            fill_bar(v, 0);
        }
    } else {
        if (M32(arg0, 8) > 0x200)
            M32(arg0, 8) = 0x200;
        else if (M32(arg0, 8) < 0)
            M32(arg0, 8) = 0;
        v = M32(arg0, 8);
        if (v > 0x100) {
            copy_longs(g_0608c304, g_0609d7b0, 8);
            fill_bar(v - 0x100, 1);
        } else {
            copy_longs(g_0608c2e4, g_0609d7b0, 8);
            fill_bar(v, 0);
        }
    }

    g_0609029c(&g_0609d7b0[3]);
    g_060997ac = 1;
}

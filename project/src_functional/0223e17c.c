#include "dh_work.h"
/* hud_rebuild_player_counters [0x0223e17c]: Builds up to six counter glyphs from actor inventory fields and renders the player row. */
#include "dh.h"
/* 0x0223e17c (callC) - rebuild + render a player HUD counter glyph list.
 * slot = entity->unk34 - 1 (player 0/1). rec = 16-byte state record at
 * g_060e0e58[slot*16]; buf = per-slot glyph-id buffer g_060e0e78[slot*8].
 *
 * Build phase: fill buf with glyph ids (stops once 6 are written):
 *   if (unk8C & 0x02000000) push 1; if (unk70) push 2;
 *   push 3 repeated unk72 times, push 4 repeated unk73 times (each push
 *   halts the build when the index passes 5). Remaining buf slots -> 0.
 * Draw phase: emit each glyph via func_022380c0(4, x, 0xD, buf[i]), unrolled
 *   3 at a time. Player 0 draws left-to-right from x=7 (+2 per glyph); player 1
 *   right-to-left from x=0x23 (-2 per glyph). Then draw the label slot
 *   func_022380c0(4, g_0608d1cc[slot], 0xC, 0) and cache rec->unk4 = total.
 * fidelity: HIGH. func_022380c0 = cart 0x2380c0 digit/icon draw (copied to
 * RAM 0x0603c0c0); 4-arg (color, x, mode, char). No braf/bsrf. */
extern u8  g_060e0e58[];
extern u8  g_060e0e78[];
extern s16 g_0608d1cc[];
extern void hud_draw_counter_icon(s32 color, s32 x, s32 mode, s32 ch);

void hud_rebuild_player_counters(void *entity) {
    s32 slot = ACTOR_M8(entity, slot_id) - 1;
    void *rec = &g_060e0e58[slot * 0x10];
    u8 *buf = &g_060e0e78[slot * 8];
    u8 *end = buf + 6;
    s32 idx = 0;
    s32 n;
    u8 *p;
    s32 x;

    if (ACTOR_M32(entity, interaction_flags) & 0x02000000)
        buf[idx++] = 1;
    if (ACTOR_M8(entity, primary_handoff_pending) != 0)
        buf[idx++] = 2;

    n = 0;
    while (n < M8(entity, 0x72)) {
        buf[idx++] = 3;
        if (idx > 5)
            break;
        n++;
    }
    n = 0;
    while (n < M8(entity, 0x73)) {
        buf[idx++] = 4;
        if (idx > 5)
            break;
        n++;
    }

    for (p = buf + idx; p < end; p++)
        *p = 0;

    if (slot == 0) {
        x = 7;
        for (p = buf; p < end; p += 3) {
            hud_draw_counter_icon(4, x, 0xD, p[0]);
            hud_draw_counter_icon(4, x + 2, 0xD, p[1]);
            hud_draw_counter_icon(4, x + 4, 0xD, p[2]);
            x += 6;
        }
    } else {
        x = 0x23;
        for (p = buf; p < end; p += 3) {
            hud_draw_counter_icon(4, x, 0xD, p[0]);
            hud_draw_counter_icon(4, x - 2, 0xD, p[1]);
            hud_draw_counter_icon(4, x - 4, 0xD, p[2]);
            x -= 6;
        }
    }

    hud_draw_counter_icon(4, g_0608d1cc[slot], 0xC, 0);
    M16(rec, 0x4) = (s16)(M8(entity, 0x72) + M8(entity, 0x73) + ACTOR_M8(entity, primary_handoff_pending));
}

#include "dh_work.h"
/* hud_update_player_counters [0x0223df30]: Advances counter blink timers and rebuilds changed player counter glyph lists. */
#include "dh.h"
/* 0x0223df30 (callC) - per-frame update + redraw of a player HUD counter.
 * slot = entity->unk34 - 1 (player 0/1). rec = 16-byte state record at
 * g_060e0e58[slot*16]: {u32 unk0 content-hash, s16 unk4 total, s16 unk8 timerA,
 * s16 unkA glyphA, s16 unkC timerB, s16 unkE glyphB}. buf = g_060e0e78[slot*8]
 * (SIGNED bytes: mov.b load sign-extends into the draw argument).
 *
 * If timerA running: blink-draw glyph buf[unkA], decrement; on reaching 0
 *   rebuild the list (func_0223e17c). Else if timerB running: decrement; on the
 *   frame it hits 0 draw buf[unkE] solid (color 4), otherwise blink-draw it.
 * Else (idle): if (flags & 0xFFE) != (unk0 & 0xFFE) run func_0223e104; else if
 *   the cached total or the high content byte changed, rebuild (func_0223e17c).
 * Blink color = (g_060987bc & 1) ? 4 : 0xF. Glyph x: player 0 -> gi*2+7,
 *   player 1 -> 0x23 - gi*2, mode 0xD.
 * Tail: if (flags & 0x39E) draw a status value via func_022380fa at
 *   g_0608d1cc[slot] (mode 0xC): obj = g_060e0f4c[unk6F]->unk8; v = obj->unk56;
 *   v>0 -> draw v solid, else blink-draw 0.
 * fidelity: HIGH. func_022380c0 (cart 0x2380c0, RAM 0x0603c0c0) glyph draw;
 * func_022380fa companion draw; func_0223e104/func_0223e17c siblings. All
 * jsr @reg indirects resolved via ptr_resolve (no braf/bsrf). */
extern u8   g_060e0e58[];
extern s8   g_060e0e78[];
extern s16  g_0608d1cc[];
extern s32  g_frame_tick_legacy;
extern void *g_060e0f4c[];
extern void hud_draw_counter_icon(s32 color, s32 x, s32 mode, s32 ch);
extern void hud_draw_indexed_label(s32 color, s32 x, s32 mode, s32 ch);
extern void hud_queue_action_label(void *entity);
extern void hud_rebuild_player_counters(void *entity);

void hud_update_player_counters(void *entity) {
    s32 slot = ACTOR_M8(entity, slot_id) - 1;
    void *rec = &g_060e0e58[slot * 0x10];
    s8 *buf = &g_060e0e78[slot * 8];
    u32 flags = ACTOR_M32(entity, interaction_flags);
    s32 color;
    s32 gi;
    s32 x;

    if (M16(rec, 0x8) != 0) {
        color = (g_frame_tick_legacy & 1) ? 4 : 0xF;
        gi = M16(rec, 0xA);
        x = (slot == 0) ? gi * 2 + 7 : 0x23 - gi * 2;
        hud_draw_counter_icon(color, x, 0xD, buf[gi]);
        M16(rec, 0x8) = (s16)(M16(rec, 0x8) - 1);
        if (M16(rec, 0x8) == 0)
            hud_rebuild_player_counters(entity);
    } else if (M16(rec, 0xC) != 0) {
        M16(rec, 0xC) = (s16)(M16(rec, 0xC) - 1);
        gi = M16(rec, 0xE);
        x = (slot == 0) ? gi * 2 + 7 : 0x23 - gi * 2;
        if (M16(rec, 0xC) == 0) {
            hud_draw_counter_icon(4, x, 0xD, buf[gi]);
        } else {
            color = (g_frame_tick_legacy & 1) ? 4 : 0xF;
            hud_draw_counter_icon(color, x, 0xD, buf[gi]);
        }
    } else {
        u32 v = M32(rec, 0x0);
        if ((flags & 0xFFE) != (v & 0xFFE)) {
            hud_queue_action_label(entity);
        } else if (M16(rec, 0x4) != (s16)(M8(entity, 0x72) + M8(entity, 0x73) + ACTOR_M8(entity, primary_handoff_pending))
                   || (s8)(v >> 24) != (s8)(flags >> 24)) {
            hud_rebuild_player_counters(entity);
        }
    }

    if (flags & 0x39E) {
        void *obj = MP(g_060e0f4c[ACTOR_M8(entity, secondary_effect_slot)], 0x8);
        s16 v16 = M16(obj, 0x56);
        x = g_0608d1cc[slot];
        if (v16 > 0) {
            hud_draw_indexed_label(4, x, 0xC, v16);
        } else {
            color = (g_frame_tick_legacy & 1) ? 4 : 0xF;
            hud_draw_indexed_label(color, x, 0xC, 0);
        }
    }
}

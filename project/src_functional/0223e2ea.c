/* hud_score_slots_init [0x0223e2ea]: Existing map corroborated by source: per-player HUD slot init; func_022380c0(4,idx,mode,0) sequence; record g_0609860c; clears g_060e0e58[idx*16] */
#include "dh.h"
/* 0x0223e2ea - per-player HUD/score slot initialiser.
 * idx = arg0->unk34 - 1 (player index).
 * Register the main slot: func_022380c0(4, g_0608d1cc[idx], 12, 0).
 * For idx < 2: set the 8-byte record g_0609860c[idx] = { unk0 = 1 (word),
 *   unk2 = 0 (word), unk4 = 12 (long) }.
 * Register the digit slots via func_022380c0(4, n, 13, 0): for idx == 0 use
 *   n = 7,9,11,13,15,17 (ascending), otherwise n = 35,33,31,29,27,25
 *   (descending). (In the ROM the slot loops are unrolled 3 calls/iteration.)
 * Finally zero the 16-byte state block g_060e0e58[idx*16].
 * fidelity: HIGH. func-ptr r13 = mova(.Lpool_23e30c)+0xFFFF9DB4 = 0x022380c0. */
extern void hud_draw_counter_icon(s32 a, s32 b, s32 c, s32 d);
extern u16 g_0608d1cc[];
extern u8  g_0609860c[];
extern u8  g_060e0e58[];

void hud_score_slots_init(void *arg0) {
    s32 idx = M8(arg0, 0x34) - 1;
    void *rec;
    s32 n;

    hud_draw_counter_icon(4, g_0608d1cc[idx], 12, 0);

    if ((u32)idx < 2) {
        rec = &g_0609860c[idx * 8];
        M16(rec, 0) = 1;
        M16(rec, 2) = 0;
        M32(rec, 4) = 12;
    }

    if (idx == 0) {
        for (n = 7; n < 19; n += 2)
            hud_draw_counter_icon(4, n, 13, 0);
    } else {
        for (n = 35; n > 23; n -= 2)
            hud_draw_counter_icon(4, n, 13, 0);
    }

    rec = &g_060e0e58[idx * 16];
    M32(rec, 0)  = 0;
    M32(rec, 4)  = 0;
    M32(rec, 8)  = 0;
    M32(rec, 12) = 0;
}

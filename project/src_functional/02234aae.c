/* gfx_submit_box8 [0x02234aae]: Existing map corroborated by source: builds a 4-corner 8x8 box record on stack, emits via func_0221566e (emit_sprite_cmd); guard g_060987BC&6 */
#include "dh.h"
/* 0x02234aae (gfx) - submit an 8x8 box (4-corner quad) centred at (x,y) to the
 * sprite pipeline, unless g_060987BC has bit 1 or 2 set. Builds a command record
 * on the stack: {word 0, word 1, 4 corner xy pairs (x+-4, y+-4), long attr/depth,
 * word 0, word 0} and passes &record to emit_sprite_cmd (emit_sprite_cmd).
 * fidelity: HIGH - coords stored as words (s16); attr long; guard mask 6. */
extern s32 g_frame_tick_legacy;
extern void emit_sprite_cmd(void *rec);

void gfx_submit_box8(s32 x, s32 y, s32 attr) {
    u8 rec[40];
    if (g_frame_tick_legacy & 6)
        return;
    M16(rec, 12) = 0;
    M16(rec, 14) = 1;
    M16(rec, 16) = (s16)(x - 4);
    M16(rec, 18) = (s16)(y - 4);
    M16(rec, 20) = (s16)(x + 4);
    M16(rec, 22) = (s16)(y - 4);
    M16(rec, 24) = (s16)(x + 4);
    M16(rec, 26) = (s16)(y + 4);
    M16(rec, 28) = (s16)(x - 4);
    M16(rec, 30) = (s16)(y + 4);
    M32(rec, 32) = attr;
    M16(rec, 36) = 0;
    M16(rec, 38) = 0;
    emit_sprite_cmd(rec + 12);
}

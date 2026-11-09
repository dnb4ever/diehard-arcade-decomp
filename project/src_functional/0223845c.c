/* sound_step_command_cursor [0x0223845c]: Advances packed cue command pointer; kind selects direct or model-indexed sound ID. */
#include "dh.h"
/* 0x0223845c - advance command record pointer at arg0+0x64 and request sound.
 * cmd = *(u32 *)(arg0 + 100); kind = (s8)cmd[1]; val = (s16)cmd[2];
 * kind 0: if g_060e0ddc != 0 sound_queue_push((val << 8) | 0xA00000FF);
 * kind 1: tbl = 0x06054080 + (s8)*(0x060540E8 + (u16)arg1[94]) * 8;
 *         if g_060e0ddc != 0
 *             req = ((u16)*(u16 *)(0x06053F04 + (u8)tbl[(u8)(s16)val] * 2) << 8)
 *                   | 0xA00000FF; sound_queue_push(req);
 * *(u32 *)(arg0 + 100) = cmd + 4; b = (s8)*(u8 *)(cmd + 4);
 * *(s32 *)(arg0 + 64) = b; return b >= 0. */
extern volatile s32 g_sound_effects_enabled;
extern void sound_queue_push(s32 req);

s32 sound_step_command_cursor(u32 arg0, u32 arg1) {
    u32 cmd, tbl;
    s16 val;
    s8 kind, b;

    cmd = MU32(arg0, 100);
    val = M16(cmd, 2);
    kind = M8(cmd, 1);

    if (kind == 0) {
        if (g_sound_effects_enabled != 0)
            sound_queue_push(((s32)val << 8) | DH_SOUND_CUE_BASE);
    } else if (kind == 1) {
        tbl = 0x06054080 + (s32)(s8)M8(0x060540E8 + MU16(arg1, 94), 0) * 8;
        if (g_sound_effects_enabled != 0)
            sound_queue_push(((u32)MU16(0x06053F04 +
                (u32)(u8)M8(tbl, (u32)(u8)(s16)val) * 2, 0) << 8) |
                DH_SOUND_CUE_BASE);
    }

    cmd += 4;
    M32(arg0, 100) = cmd;
    b = M8(cmd, 0);
    M32(arg0, 64) = (s32)b;
    return (s32)b >= 0;
}

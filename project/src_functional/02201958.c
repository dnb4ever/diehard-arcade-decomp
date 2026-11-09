/* game_reset_globals [0x02201958]: Clears game globals, reads BIOS configuration and enables sound. */
#include "dh.h"
/* 0x02201958 (boot global-init, FUN_02201958) - called by main_init_entry. Clears the HWRAM
 * game-state globals to their reset values, then probes the machine/region:
 *  - calls the BIOS callback at *0x0600063C with (3, &g_06098674), stashing its result in
 *    g_06098804 (a BIOS device/slot query);
 *  - decodes the BIOS config word g_06000248: g_06098808 = region table g_060511DC[(cfg>>12)&0xF] * 2;
 *  - runs smpc_intback_read() (g_06098800 = result) and the BIOS callback at *0x06000634 on the
 *    INTBACK result buffer g_060987EC;
 *  - decodes a byte of g_060987EC (at index g_06098808): language g_06099794 = g_060511CC[(b>>3)&7],
 *    player/difficulty g_06099796 = ((b>>6)&3)+1;
 *  - sets the sound-enable g_060E0DDC = 1 and various state flags, zeroes the rest.
 * fidelity: HIGH - widths from the stores (byte/word/long as noted); BIOS calls are indirect
 * through the fixed pointers at 0x0600063C / 0x06000634; config decode shifts/masks verified
 * against the asm. computed call resolved via braf_rewrite.py (bsrf -> func_0220acf6). */
extern s32  smpc_read_intback(void);                  /* smpc_intback_read */
extern s32  (*g_0600063C)(s32, void *);           /* BIOS callback slot */
extern void (*g_06000634)(void *);                /* BIOS callback slot */
extern u32  g_06000248;                           /* BIOS config word */
extern s32  g_060511DC[];                         /* region stride table */
extern s16  g_060511CC[];                         /* language table */
extern u8   g_bios_status_buffer[];                         /* SMPC INTBACK result buffer */
extern u8   g_player_join_states[];                         /* 4-byte state block */
extern u8   g_060987D4[];                         /* 4-byte state block */
extern u8   g_06098688[];                         /* small byte-flag block */

extern s32  g_frame_tick, g_060987C0, g_06098698, g_0609869C, g_06098828;
extern s32  g_06096C90, g_06099578, g_0609977C, g_060987C8;
extern s32  g_06098804, g_06098808, g_06098800, g_06098694, g_06098690;
extern s32  g_060987D0, g_sound_effects_enabled_legacy, g_play_session_mode, g_ready_player_mask, g_scene_index;
extern s32  g_scene_event_flags, g_audio_command_ready, g_060986A8;
extern u8   g_060986AC, g_060986AD, g_060986B1, g_060986AE, g_0609861C;
extern u8   g_0609868C, g_0609868D;
extern s16  g_06099794, g_06099796, g_06098810, g_0609880C, g_06098812, g_0609880E;

void game_reset_globals(void) {
    u8 b;
    s32 idx;

    g_frame_tick = 0;
    g_060987C0 = 0;
    g_06098698 = 1;
    g_0609869C = 0;
    g_06098828 = 0;
    g_060986AC = 0;
    g_060986AD = 1;
    g_060986B1 = 0;
    g_060986AE = 1;
    g_0609861C = 0;
    g_06096C90 = 0;
    g_06099578 = 0;
    M8(g_06098688, 3) = 0;
    M8(g_06098688, 2) = 0;
    M8(g_06098688, 1) = 0;
    M8(g_06098688, 0) = 0;
    g_0609977C = 0;
    g_060987C8 = 3;
    M8(g_060987D4, 3) = 0;
    M8(g_060987D4, 2) = 0;
    M8(g_060987D4, 1) = 0;
    M8(g_060987D4, 0) = 0;
    M8(g_player_join_states, 3) = 0;
    M8(g_player_join_states, 2) = 0;
    M8(g_player_join_states, 1) = 0;
    M8(g_player_join_states, 0) = 0;
    g_06098804 = g_0600063C(3, g_player_join_states);
    g_06098808 = g_060511DC[(g_06000248 >> 0xC) & 0xF] * 2;
    g_06098800 = smpc_read_intback();
    g_06000634(g_bios_status_buffer);
    g_06098694 = 0;
    g_06098690 = 0;
    idx = g_06098808;
    b = g_bios_status_buffer[idx];
    g_06099794 = g_060511CC[(b >> 3) & 7];
    g_06099796 = (s16)(((b >> 6) & 3) + 1);
    g_060987D0 = 1;
    g_sound_effects_enabled_legacy = 1;
    g_play_session_mode = 0;
    g_ready_player_mask = 0;
    g_scene_index = DH_SCENE_SELECTOR;
    g_06098810 = 0;
    g_0609880C = 0;
    g_06098812 = 0;
    g_0609880E = 0;
    g_scene_event_flags = 0;
    g_audio_command_ready = 0;
    g_0609868D = 0;
    g_0609868C = 0;
    g_060986A8 = 0;
}

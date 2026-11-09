/* player_refresh_ready_mask [0x0222a8d8]: Rebuilds the two-player readiness mask and returns the ready count. */
#include "dh.h"
/* 0x0222a8d8  compute player-ready status; writes flags to 0x06098564; returns ready count (0..2) or 2.
 * fidelity: HIGH - exact; 0x06098674[0],[1] read as unsigned bytes (extu.b). */
extern s32 g_ready_player_mask;
extern s32 g_play_session_mode;
extern u8  g_player_join_states[];
s32 player_refresh_ready_mask(void) {
    s32 n = 0;
    g_ready_player_mask = 0;
    if (g_play_session_mode == 0) {
        g_ready_player_mask |= 3;
        return 2;
    }
    if (g_player_join_states[0] == 2) {
        n += 1;
        g_ready_player_mask |= 1;
    }
    if (g_player_join_states[1] == 2) {
        n += 1;
        g_ready_player_mask |= 2;
    }
    return n;
}

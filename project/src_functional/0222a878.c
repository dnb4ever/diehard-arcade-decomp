/* scene_select_player_parameter [0x0222a878]: Updates player selection and chooses descriptor+0x1c record word 0,4,8 by player mask/count. */
#include "dh.h"
/* 0x0222a878 scene_value_select(void) - resolves a value from the current scene
 * descriptor's unk1C sub-record, based on the player-count mode. Returns s32.
 *   g_06098564 = 0;
 *   if (g_06098670 == 1) {                 (two-player present)
 *       if (g_06098674[0] == 1) g_06098674[0] = 2;
 *       if (g_06098674[1] == 1) g_06098674[1] = 2;
 *       sel = func_0222a8d8();
 *   } else { sel = 2; g_06098564 = 3; }
 *   rec = (*g_0609866C)->unk1C;
 *   if (sel == 1) return (g_06098564 & 1) ? rec->unk0 : rec->unk4;
 *   else          return rec->unk8;
 * fidelity: HIGH - func_0222a8d8 is void/void (existing file); g_0609866C holds the
 * scene descriptor pointer; delay-slot seed sel=2; matches m2c. */
extern s32   g_ready_player_mask;
extern s32   g_play_session_mode;
extern u8    g_player_join_states[];
extern void *g_scene_descriptor;
extern s32   player_refresh_ready_mask(void);

s32 scene_select_player_parameter(void) {
    void *rec;
    s32 sel;

    g_ready_player_mask = 0;
    if (g_play_session_mode == 1) {
        if (g_player_join_states[0] == 1) {
            g_player_join_states[0] = 2;
        }
        if (g_player_join_states[1] == 1) {
            g_player_join_states[1] = 2;
        }
        sel = player_refresh_ready_mask();
    } else {
        sel = 2;
        g_ready_player_mask = 3;
    }
    rec = MP(g_scene_descriptor, 0x1C);
    if (sel == 1) {
        return (g_ready_player_mask & 1) ? M32(rec, 0x0) : M32(rec, 0x4);
    }
    return M32(rec, 0x8);
}

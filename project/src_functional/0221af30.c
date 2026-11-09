/* Round 4: ROM 0601ef4c loads registry base 06099584, then directly loads base+30/+34; declare address array, not pointer-valued cell. Evidence: work/decomp4/pointer_tail.asm. */
/* player_update_aim_limits [0x0221af30]: Adjusts the two HUD/player parameter words using scene deltas, limits and readiness gates. */
#include "dh.h"
/* 0x0221af30 (callC) - apply a per-frame aim/turn delta to the crosshair pair at g_06099788
 * and mirror it into the active player object's field56.
 * delta = (s16)g_060543E0[ g_06098668 (+0x52 unless g_06098670 == 1) ].
 * When delta > 0 the turn is clamped to the ceiling g_06099794*2 and routed by g_0609856C bit1
 * (vertical crosshair[2]+field34 vs horizontal crosshair[0]+field30). When delta <= 0 the turn is
 * floored at 1 and routed by input bits: for arg0 == 1 via g_0609856C bits 0x10000/0x20000,
 * otherwise via g_06098568 bits 1/2.
 *   horizontal: crosshair[0] -> g_06099584->field30->8->field56;
 *   vertical:   crosshair[2] -> g_06099584->field34->8->field56.
 * fidelity: HIGH - crosshair/limit/player field56 16-bit; delta s16 sign-extended; input 32-bit. */
extern s32   g_scene_index;
extern s32   g_play_session_mode;
extern s16   g_060543E0[];
extern s16   g_06099788;
extern s16   g_06099794;
extern s32   g_0609856C;
extern s32   g_06098568;
extern u8 g_actor_slot_registry[];
void player_update_aim_limits(s32 arg0) {
    s32 base = g_scene_index;
    s32 idx = (g_play_session_mode == 1) ? base : base + 0x52;
    s32 delta = (s16)g_060543E0[idx];
    if (delta > 0) {
        s32 hi = (s32)g_06099794 * 2;
        if (g_0609856C & 2) {
            M16(&g_06099788, 2) = (s16)(M16(&g_06099788, 2) + delta);
            if (M16(&g_06099788, 2) > hi)
                M16(&g_06099788, 2) = (s16)hi;
            M16(MP(MP(g_actor_slot_registry, 0x34), 0x8), 0x56) = M16(&g_06099788, 2);
        } else {
            M16(&g_06099788, 0) = (s16)(M16(&g_06099788, 0) + delta);
            if (M16(&g_06099788, 0) > hi)
                M16(&g_06099788, 0) = (s16)hi;
            M16(MP(MP(g_actor_slot_registry, 0x30), 0x8), 0x56) = M16(&g_06099788, 0);
        }
    } else if (arg0 == 1) {
        if (g_0609856C & 0x10000) {
            M16(&g_06099788, 0) = (s16)(M16(&g_06099788, 0) + delta);
            if (M16(&g_06099788, 0) < 1)
                M16(&g_06099788, 0) = 1;
            M16(MP(MP(g_actor_slot_registry, 0x30), 0x8), 0x56) = M16(&g_06099788, 0);
        }
        if (g_0609856C & 0x20000) {
            M16(&g_06099788, 2) = (s16)(M16(&g_06099788, 2) + delta);
            if (M16(&g_06099788, 2) < 1)
                M16(&g_06099788, 2) = 1;
            M16(MP(MP(g_actor_slot_registry, 0x34), 0x8), 0x56) = M16(&g_06099788, 2);
        }
    } else {
        if (g_06098568 & 1) {
            M16(&g_06099788, 0) = (s16)(M16(&g_06099788, 0) + delta);
            if (M16(&g_06099788, 0) < 1)
                M16(&g_06099788, 0) = 1;
            M16(MP(MP(g_actor_slot_registry, 0x30), 0x8), 0x56) = M16(&g_06099788, 0);
        }
        if (g_06098568 & 2) {
            M16(&g_06099788, 2) = (s16)(M16(&g_06099788, 2) + delta);
            if (M16(&g_06099788, 2) < 1)
                M16(&g_06099788, 2) = 1;
            M16(MP(MP(g_actor_slot_registry, 0x34), 0x8), 0x56) = M16(&g_06099788, 2);
        }
    }
}

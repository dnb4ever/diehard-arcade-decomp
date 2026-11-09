#include "dh_work.h"
/* enemy_enter_stage_pose18 [0x02249f08]: Enters state eighteen with the stage-selected pose and conditional timing flags. */
#include "dh.h"
/* 0x02249f08  enter actor state 18: field[0xd4]=18; field[0x84]=1; field[0x88]=0;
 * field[0xe0] &= 0x300; field[0x108]=0; field[0x41]=1;
 * field[0x5c] = g_060900e0[g_06097418];
 * if (g_06097418 == 1) field[0xbc] |= 0x7FFFFF00.
 * fidelity: HIGH - exact (g_060900e0 indexed as s16; shll = *2 word stride). */
extern s32 g_06097418;
extern s16 g_060900e0[];
void enemy_enter_stage_pose18(void *p) {
    ACTOR_M32(p, behavior_state) = DH_ENEMY_BEHAVIOR_POSE;
    ACTOR_M32(p, action_category) = 1;
    ACTOR_M32(p, action_state) = 0;
    ACTOR_M32(p, state_flags) &= 0x300;
    ACTOR_M32(p, action_phase) = 0;
    ACTOR_M8(p, animation_control) = DH_ANIMATION_CONTROL_RESTART;
    ACTOR_M16(p, animation_requested) = g_060900e0[g_06097418];
    if (g_06097418 == 1)
        M32(p, 0xBC) |= 0x7FFFFF00;
}

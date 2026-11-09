/* enemy_load_action_descriptor_source_alias [0x02245c76]: Duplicate loader source bound to 02245c88 by v1.09; copy order +184 before +180 retained. */
#include "dh_work.h"
#include "dh.h"
/* 0x02245c76  copy an animation/pose record into actor fields 0x148..0x188.
 * s = g_06091dd4[field[0xe4]][field[0x6d]] (two-level pointer-table lookup,
 * same as func_02241e06). First five dst are 16-bit, rest 32-bit; note dst
 * order 0x184 then 0x180 (faithful to the instruction stream).
 * fidelity: HIGH - exact (the 0xffff/0x0609 bytes at 0x245c76 are the prior
 * function's literal pool; the real body begins at 0x245c86). */
extern void *g_06091dd4[];
void enemy_load_action_descriptor_source_alias(void *p) {
    void **base = (void **)g_06091dd4[ACTOR_M32(p, state_index)];
    void *s = base[(s8)ACTOR_M8(p, secondary_effect_type)];
    ACTOR_M16(p, idle_animation) = M16(s, 0);
    ACTOR_M16(p, approach_animation) = M16(s, 2);
    ACTOR_M16(p, state2_animation) = M16(s, 4);
    ACTOR_M16(p, state8_animation) = M16(s, 6);
    M16(p, 0x150) = M16(s, 8);
    ACTOR_M32(p, direction_animation_table) = M32(s, 12);
    ACTOR_M32(p, action_config_tables) = M32(s, 16);
    ACTOR_M32(p, reaction_config_tables) = M32(s, 20);
    M32(p, 0x160) = M32(s, 24);
    ACTOR_M32(p, response_config_tables) = M32(s, 28);
    ACTOR_M32(p, distance_attack_table) = M32(s, 32);
    ACTOR_M32(p, motion_action_tables) = M32(s, 36);
    M32(p, 0x170) = M32(s, 40);
    M32(p, 0x174) = M32(s, 44);
    M32(p, 0x178) = M32(s, 48);
    M32(p, 0x17C) = M32(s, 52);
    ACTOR_M32(p, state_animation_descriptor) = M32(s, 56);
    ACTOR_M32(p, reaction_descriptor) = M32(s, 60);
    ACTOR_M32(p, variant_animation_table) = M32(s, 64);
}

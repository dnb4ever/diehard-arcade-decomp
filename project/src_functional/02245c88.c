#include "dh_work.h"
/* enemy_load_action_descriptor [0x02245c88]: Copies the state/variant-selected animation and action descriptor into the actor tail. */
#include "dh.h"
/* 0x02245c88 - func_02245c88(entity): load an animation/action descriptor into
 * the entity's 0x148.. block. The source record is selected by two indices:
 *   rec = g_06091dd4[entity->0xE4];           // table of pointer arrays
 *   src = rec[entity->0x6D];                   // pointer to the descriptor
 * then a fixed field-by-field copy of five u16 and fourteen s32 words:
 *   0x148=src[0] 0x14A=src[2] 0x14C=src[4] 0x14E=src[6] 0x150=src[8]  (u16)
 *   0x154=*(src+12) 0x158=*(src+16) 0x15C=*(src+20) 0x160=*(src+24)
 *   0x164=*(src+28) 0x168=*(src+32) 0x16C=*(src+36) 0x170=*(src+40)
 *   0x174=*(src+44) 0x178=*(src+48) 0x17C=*(src+52)
 *   0x184=*(src+56) 0x180=*(src+60) 0x188=*(src+64)                   (s32)
 * fidelity: HIGH - pure load/store copy, no calls. Index entity->0x6D read as
 * signed byte (mov.b); entity->0xE4 as s32. The destination order mirrors the
 * asm: +0x184 is written before +0x180 (the pointer walks +8 then back -4).
 * The caller (func_02240054) passes a 2nd arg in r5; this function overwrites
 * r5 immediately and never uses it, so the real signature takes only entity. */
extern void **g_06091dd4[];                    /* table of descriptor-pointer arrays */

void enemy_load_action_descriptor(void *entity)
{
    void **rec = g_06091dd4[ACTOR_M32(entity, state_index)];
    void *src  = rec[ACTOR_M8(entity, secondary_effect_type)];

    ACTOR_M16(entity, idle_animation) = M16(src, 0);
    ACTOR_M16(entity, approach_animation) = M16(src, 2);
    ACTOR_M16(entity, state2_animation) = M16(src, 4);
    ACTOR_M16(entity, state8_animation) = M16(src, 6);
    M16(entity, 0x150) = M16(src, 8);

    ACTOR_M32(entity, direction_animation_table) = M32(src, 12);
    ACTOR_M32(entity, action_config_tables) = M32(src, 16);
    ACTOR_M32(entity, reaction_config_tables) = M32(src, 20);
    M32(entity, 0x160) = M32(src, 24);
    ACTOR_M32(entity, response_config_tables) = M32(src, 28);
    ACTOR_M32(entity, distance_attack_table) = M32(src, 32);
    ACTOR_M32(entity, motion_action_tables) = M32(src, 36);
    M32(entity, 0x170) = M32(src, 40);
    M32(entity, 0x174) = M32(src, 44);
    M32(entity, 0x178) = M32(src, 48);
    M32(entity, 0x17C) = M32(src, 52);
    ACTOR_M32(entity, state_animation_descriptor) = M32(src, 56);
    ACTOR_M32(entity, reaction_descriptor) = M32(src, 60);
    ACTOR_M32(entity, variant_animation_table) = M32(src, 64);
}

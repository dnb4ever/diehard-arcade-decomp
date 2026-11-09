/* enemy_enter_configured_step [0x02241d18]: Behavior state 4/phase 2, actor flags and direction pick config-table move. */
#include "dh.h"
/* 0x02241d18 - func_02241d18(entity): initialise actor from a config table entry.
 *   entity->0xD4 = 4; entity->0xD8 = 2;
 *   entity->0x84 = 0x400; entity->0xE0 = 0x00100000;
 *   entity->0xC4 = ((~entity->0xE8) & 7) * 2;
 *   entity->0x41 = 1;
 *   cond = (entity->0x124 & 2) ? 1 : 0;
 *   if (entity->0x44 != 0) cond ^= 1;
 *   actor_init_from_cfg(entity, *(s16 **)(0x06091924 + cond * 4));
 * fidelity: HIGH - the 0xE8 word is read after the 0xE0 store and NOT'ed;
 * the braf delay slot pops r14 (stack restore) before the tail call; the
 * config pointer comes from the word table at 0x06091924 indexed by cond<<2. */
void actor_init_from_cfg(u8 *actor, s16 *src);

void enemy_enter_configured_step(u8 *entity)
{
    u32 cond;

    M32(entity, 0xD4) = DH_ENEMY_BEHAVIOR_GUARD;
    M32(entity, 0xD8) = 2;
    M32(entity, 0x84) = 4 << 8;
    M32(entity, 0xE0) = 0x00100000;
    M32(entity, 0xC4) = ((~MU32(entity, 0xE8)) & 7) << 1;
    M8(entity, 0x41) = 1;

    cond = (M32(entity, 0x124) & 2) ? 1 : 0;
    if (M8(entity, 0x44) != 0)
        cond ^= 1;

    actor_init_from_cfg(entity, *(s16 **)(uintptr_t)(0x06091924 + (cond << 2)));
}

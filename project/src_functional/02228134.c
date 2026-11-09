/* players_entities_ready [0x02228134]: Existing map corroborated by source: leaf (member) returns -1 if any active player's actor */
#include "dh.h"
/* 0x02228134 players_entities_ready(void) - returns -1 if any active player's actor
 * entity reports not-ready (entity->unk56 == 0), else 0. g = g_06099584:
 *   if (g->unk10 && ((actor g->unk30)->unk8)->unk56 == 0) return -1;   (player 1)
 *   if (g->unk14 && ((actor g->unk34)->unk8)->unk56 == 0) return -1;   (player 2)
 *   return 0;
 * fidelity: HIGH - leaf, no calls/computed branches; matches m2c. */
extern u8 g_actor_slot_registry[];   /* two-player control block */

s32 players_entities_ready(void) {
    void *actor;
    void *entity;

    if (M32(g_actor_slot_registry, 0x10) != 0) {
        actor = MP(g_actor_slot_registry, 0x30);
        entity = MP(actor, 0x8);
        if (M16(entity, 0x56) == 0) {
            return -1;
        }
    }
    if (M32(g_actor_slot_registry, 0x14) != 0) {
        actor = MP(g_actor_slot_registry, 0x34);
        entity = MP(actor, 0x8);
        if (M16(entity, 0x56) == 0) {
            return -1;
        }
    }
    return 0;
}

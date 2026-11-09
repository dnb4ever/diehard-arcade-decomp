/* both_players_special_trigger_source_alias [0x0223de0e]: Duplicate draft bound by v1.09 to 0223de20; triggers eligible active players. */
#include "dh.h"
/* 0x0223de0e (callC) - trigger a special move/state on both active players.
 * ctx = g_06099584; player 0 = ctx->unk30->unk8 (gated by ctx->unk10),
 * player 1 = ctx->unk34->unk8 (gated by ctx->unk14).
 * For each eligible player entity whose (unk8C & 0xFFFFFF) is set and
 * (unk8C & 2) is clear: clear unk74, set unk62 = unk44 ? 0xC000 : 0x4000,
 * unk60 = 0x2000, clear unk45; then invoke func_0223a9d0(entity, 3). When the
 * entity is in state unk6C == 0xC it is momentarily forced to 0 across the call
 * and restored to 0xC afterwards.
 * fidelity: HIGH. bsrf/braf resolved to func_0223a9d0. */
extern u8   g_actor_slot_registry;
extern void actor_commit_effect_handoffs(void *entity, s32 a);

static void de0e_trigger(void *ent) {
    s32 flags = M32(ent, 0x8C);

    if ((flags & 0xFFFFFF) == 0 || (flags & 2) != 0)
        return;

    M32(ent, 0x74) = 0;
    M16(ent, 0x62) = M8(ent, 0x44) ? (s16)0xC000 : (s16)0x4000;
    M16(ent, 0x60) = 0x2000;
    M8(ent, 0x45) = 0;

    if (M8(ent, 0x6C) == 0xC) {
        M8(ent, 0x6C) = 0;
        actor_commit_effect_handoffs(ent, 3);
        M8(ent, 0x6C) = 0xC;
    } else {
        actor_commit_effect_handoffs(ent, 3);
    }
}

void both_players_special_trigger_source_alias(void) {
    if (M32(&g_actor_slot_registry, 0x10) != 0)
        de0e_trigger(MP(MP(&g_actor_slot_registry, 0x30), 0x8));
    if (M32(&g_actor_slot_registry, 0x14) != 0)
        de0e_trigger(MP(MP(&g_actor_slot_registry, 0x34), 0x8));
}

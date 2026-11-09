/* both_players_special_trigger [0x0223de20]: Existing map corroborated by source: trigger special state on both active players (ctx g_06099584, p0=unk30->unk8 gated unk10, p1=unk34->unk8 gated unk14). per entity with (unk8C&0xFFFFFF) set & (unk8C&2) clear: unk74=0, unk62=unk44?0xC000:0x4000, unk60=0x2000, unk45=0, func_0223a9d0(entity,3) (unk6C==0xC forced to 0 across call then restored). bsrf/braf resolved. ; v1.09 corrected draft binding, work/decomp5/entries.asm */
#include "dh.h"
/* 0x0223de20 (no args) - per-frame refresh of both players' active special state.
 * pm = g_06099584 (player manager). For each active player (P1: pm->unk10 with
 * actor pm->unk30->unk8; P2: pm->unk14 with actor pm->unk34->unk8), when the
 * actor has state bits set (unk8C & 0xFFFFFF) but not bit 2:
 *   clear unk74; set facing unk62 = (unk44 ? 0xC000 : 0x4000) and unk60 = 0x2000;
 *   clear unk45; (re)invoke func_0223a9d0(actor, 3). If unk6C == 12, it is
 *   temporarily cleared across the call and restored afterwards.
 * fidelity: HIGH. bsrf/braf all resolve to func_0223a9d0; unk6C/unk44/unk45 byte,
 * unk60/unk62 word, unk74/unk8C long. */
extern u8   g_actor_slot_registry[];
extern void actor_commit_effect_handoffs(void *entity, s32 mode);

static void player_special_refresh(void *ent) {
    s32 flags = M32(ent, 0x8C);
    if (!(flags & 0xFFFFFF) || (flags & 2))
        return;
    M32(ent, 0x74) = 0;
    M16(ent, 0x62) = (s16)(M8(ent, 0x44) != 0 ? 0xC000 : 0x4000);
    M16(ent, 0x60) = 0x2000;
    M8(ent, 0x45) = 0;
    if (M8(ent, 0x6C) == 12) {
        M8(ent, 0x6C) = 0;
        actor_commit_effect_handoffs(ent, 3);
        M8(ent, 0x6C) = 12;
    } else {
        actor_commit_effect_handoffs(ent, 3);
    }
}

void both_players_special_trigger(void) {
    if (M32(g_actor_slot_registry, 0x10) != 0)
        player_special_refresh(MP(MP(g_actor_slot_registry, 0x30), 8));
    if (M32(g_actor_slot_registry, 0x14) != 0)
        player_special_refresh(MP(MP(g_actor_slot_registry, 0x34), 8));
}

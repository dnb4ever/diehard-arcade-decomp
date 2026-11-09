#include "dh_work.h"
/* player_save_action [0x0223dd28]: Publishes actor counts, channel flags and inventory into the player's saved action record. */
#include "dh.h"
/* 0x0223dd28 actor_status_publish(slot) - publish one actor's live state into its
 * 16-byte status record at g_060E0E38[slot-1]. slot is 1-based. Reads the actor via
 * g_060995B0[slot]; if null, no-op. entity = actor->unk8.
 *   g_06099788[slot-1] (word) = entity->unk56
 *   rec->unk2 (byte)          = entity->unk6C ? 1 : entity->unk70
 *   rec->unkA (word)          = (entity->unk6D == 1) ? 1 : 0
 *   rec->unk1 (byte)          = entity->unk6A
 *   rec->unk6 (word)          = entity->unk72
 *   rec->unk8 (word)          = entity->unk73
 *   rec->unkC (long)          = entity->unk8C & 0xFF000000
 *   rec->unk0 (byte)          = 1  (mark valid)
 * fidelity: HIGH - leaf, no calls/computed branches; matches m2c statement-for-statement. */
extern void *g_actor_slot_tasks[];     /* actor pointer table (1-based) */
extern u8    g_060E0E38[];     /* status records, 0x10 stride */
extern s16   g_06099788[];

void player_save_action(s32 slot) {
    void *actor = g_actor_slot_tasks[slot];
    void *entity;
    void *rec;
    s32 i;

    if (actor == 0) {
        return;
    }
    entity = MP(actor, 0x8);
    i = slot - 1;
    rec = &g_060E0E38[i * 0x10];
    g_06099788[i] = ACTOR_M16(entity, health);
    M8(rec, 0x2) = ACTOR_M8(entity, primary_handoff_pending);
    if (ACTOR_M8(entity, primary_effect_type) != 0) {
        M8(rec, 0x2) = 1;
    }
    M16(rec, 0xA) = (ACTOR_M8(entity, secondary_effect_type) == 1) ? 1 : 0;
    M8(rec, 0x1) = ACTOR_M8(entity, saved_primary_effect_type);
    M16(rec, 0x6) = M8(entity, 0x72);
    M16(rec, 0x8) = M8(entity, 0x73);
    M32(rec, 0xC) = ACTOR_M32(entity, interaction_flags) & 0xFF000000;
    M8(rec, 0x0) = 1;
}

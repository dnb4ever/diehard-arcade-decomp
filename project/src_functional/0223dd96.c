/* player_restore_saved_action [0x0223dd96]: Restores inventory/action fields from the player's sixteen-byte saved action record. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223dd96 - apply a queued action record to the actor player[arg0].
 * rec = &g_060e0e38[(arg0-1)*16]; ent = g_060995b0[arg0]->unk8.
 * Acts only when rec[0] (byte) == 1:
 *   ent->unk6A = rec[1]; ent->unk70 = rec[2]; ent->unk72 = (byte)rec[6] (word);
 *   ent->unk73 = (byte)rec[8] (word); ent->unk8C = rec[12] (long).
 *   If rec[10] (word) != 0: func_0223cfe4(ent, rec[10]).
 *   If ent->unk6A != 0: func_0223cf70(ent, 1).
 *   If ent->unk70 != 0: tail func_0223cf70(ent, 12).
 * fidelity: HIGH. m2c failed on bsr targets; hand-decoded. g_060995b0 stride 4,
 * g_060e0e38 stride 16. */
extern void *g_actor_slot_tasks_legacy[];
extern u8    g_060e0e38[];
extern s32   actor_begin_effect_action(void *entity, s32 action);
extern void  actor_attach_effect_slot(void *entity, s32 flag);

void player_restore_saved_action(s32 arg0) {
    void *rec = &g_060e0e38[(arg0 - 1) * 16];
    void *ent;

    if (M8(rec, 0) != 1)
        return;
    ent = TASK_PTR(g_actor_slot_tasks_legacy[arg0], work);
    ACTOR_M8(ent, saved_primary_effect_type) = M8(rec, 1);
    ACTOR_M8(ent, primary_handoff_pending) = M8(rec, 2);
    M8(ent, 0x72) = (s8)M16(rec, 6);
    M8(ent, 0x73) = (s8)M16(rec, 8);
    ACTOR_M32(ent, interaction_flags) = M32(rec, 12);
    if (M16(rec, 10) != 0)
        actor_begin_effect_action(ent, M16(rec, 10));
    if (ACTOR_M8(ent, saved_primary_effect_type) != 0)
        actor_attach_effect_slot(ent, 1);
    if (ACTOR_M8(ent, primary_handoff_pending) != 0)
        actor_attach_effect_slot(ent, 12);
}

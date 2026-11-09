/* pickup_slot_task [0x0223c650]: Existing map corroborated by source: pickup/grenade: alloc HUD slot func_02228106 -> unk34; on consume free g_060996D4[unk34-8] & next -> func_0220fe98; else func_0222eee4 */
#include "dh_work.h"
#include "dh.h"
/* 0x0223c650 (task handler) - pickup/grenade with an allocated HUD slot.
 * entity = task->unk8. First tick (task->unk10 == 0): clear entity->unkBC, grab a
 * free slot (func_02228106 -> entity->unk34 = idx+8), unk38 = 0x100000, unkA8 =
 * 0xC000, ++task->unk10. Each tick: if entity->unk3C != 0 it is being consumed ->
 * free the slot (g_060996D4[unk34-8]=0 when unk34>8) and advance task->unk0 to
 * func_0220FE98; otherwise tail-call per-frame handler func_0222EEE4(entity).
 * fidelity: HIGH - unk10/unk34 byte, unk3C/unkBC/unk38/unkA8 long.
 * func_02228106 returns the slot index+8; bsrf 0x228106; braf 0x22eee4 ->
 * func_0222eee4; next-handler ptr resolved 0x23c650 -> 0x0220fe98. */
extern s32  actor_allocate_effect_slot(void);
extern void collision_insert_grid(void *entity);
extern void task_cancel_word(void *task);
extern u8   g_effect_slot_occupied_legacy[];

void pickup_slot_task(void *task) {
    void *ent = TASK_PTR(task, work);
    if (TASK_S8(task, state) == 0) {
        M32(ent, 0xBC) = 0;
        M8(ent, 0x34) = (s8)actor_allocate_effect_slot();
        M32(ent, 0x38) = 0x100000;
        M32(ent, 0xA8) = 0xC000;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    if (M32(ent, 0x3C) != 0) {
        s32 slot = M8(ent, 0x34);
        if (slot > 8)
            g_effect_slot_occupied_legacy[slot - 8] = 0;
        TASK_PTR(task, handler) = (void *)task_cancel_word;
        return;
    }
    collision_insert_grid(ent);
}

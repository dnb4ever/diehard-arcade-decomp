/* projectile_seed_task [0x02230434]: Existing map corroborated by source: task init: seed projectile from source actor (slot/flags/launch vec g_06087F84); next -> func_022304b4 */
#include "dh_work.h"
#include "dh.h"
/* 0x02230434 (task handler, init state) - seed a projectile from its source actor.
 * entity = task->unk8; src = task->unk14->unk8.
 * Copy src->unk34 (slot id) and src->unk38 into entity; OR entity->unk68 with the
 * per-slot flag word g_0604EBE4[src->unk34]; mask entity->unk62 to 0x3F.
 * idx = (entity->unk62 - 0xB)*4 + src->unk44*2; load launch vectors from
 * g_06087F84[idx] -> entity->unk14 (long) and g_06087F84[idx+1] -> entity->unk22 (word).
 * If g_060E0DDC != 0 fire sound sound_queue_push(0xA0140CFF). Advance task->unk0 to
 * next state func_022304b4.
 * fidelity: HIGH - unk34 byte, unk38/unk68/unk14 long, unk62/unk22 word; tables s32.
 * bsrf cart 0x215f6c -> sound_queue_push; next-handler ptr resolved 0x230434 -> 0x022304b4. */
extern s32 g_actor_slot_masks[];
extern s32 g_06087f84[];
extern s32 g_sound_effects_enabled;
extern void sound_queue_push(s32 cmd);        /* sound_queue_push */
extern void projectile_emit_task(void *task);

void projectile_seed_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);
    s32 idx;
    M8(ent, 0x34)  = M8(src, 0x34);
    M32(ent, 0x38) = M32(src, 0x38);
    M32(ent, 0x68) |= g_actor_slot_masks[M8(ent, 0x34)];
    M16(ent, 0x62) = (s16)(M16(ent, 0x62) & 0x3F);
    idx = (M16(ent, 0x62) - 0xB) * 4 + M8(src, 0x44) * 2;
    M32(ent, 0x14) = g_06087f84[idx];
    M16(ent, 0x22) = (s16)g_06087f84[idx + 1];
    if (g_sound_effects_enabled != 0)
        sound_queue_push(0xA0140CFF);
    TASK_PTR(task, handler) = (void *)projectile_emit_task;
}

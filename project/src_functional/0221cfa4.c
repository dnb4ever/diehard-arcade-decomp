/* event_spawn_sequence_draft [0x0221cfa4]: Incomplete ten-state event draft, per v1.09; name describes source role, not completeness. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221cfa4 evt_actor_spawn_seq - scripted-event actor step (task FSM).
 * arg0 (r4) = task; task->unk8 = entity; task->unk10 = state index (0-9).
 * arg1 (r5) = value stored into the selected sub-table slot;
 * arg2 (r6) = s16* selector chain index into entity.
 * Any state >= 0xA returns immediately (default tail at 0x21d164).
 * Body (resolved from the braf targets / m2c --asm hint):
 *   ptr = *(u32 *)(entity + (s16)*(entity + (s16)*(entity + *arg3)));
 *   *(s32 *)((state * 2) + ptr) = arg2;          (bsrf r3 resolves scene/table call)
 *   entity->unk30 = ptr;                          (delay-slot store)
 *   process_spawn_list(entity);                   (bsrf r2)
 *   entity->unk18 = *(s32 *)((u32)entity->unk30 + 4);
 *   entity->unk1C = (*g_06098570 * 8) + 6;
 *   further bsrf r2/r3 callees (spawn/sfx helpers as in sibling steps);
 *   task->unk10++ (advance state).
 * fidelity: MEDIUM - the braf/bsrf targets are not in the supplied reachable
 * listing; callees inferred from the sibling family (0x21ce3e/0x21ce6e/0x21cee8). */

extern s32  scene_select_player_parameter(void);              /* scene_value_select */
extern void process_spawn_list(void *entity); /* 0x22a5d0 */
extern void sound_queue_push(u32 id);         /* 0x215f6c sfx trigger */
extern volatile s32 g_06098570;               /* selector table base */

void event_spawn_sequence_draft(void *task, s32 arg2, s16 *arg3) {
    void *entity = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);
    u32 ptr;
    s32 sel;

    if ((u32)(u8)state >= 0xAU) {
        return;
    }

    sel = (s32)*(s16 *)arg3;                         /* selector chain into entity */
    sel = (s32)(s16)MU16(entity, sel);
    sel = (s32)(s16)MU16(entity, sel);
    ptr = MU32(entity, sel);

    M32((void *)(ptr + (u32)(state * 2)), 0x0) = arg2; /* slot store before call */

    M32(entity, 0x30) = (s32)ptr;                    /* delay-slot store of bsrf r3 */
    process_spawn_list(entity);

    M32(entity, 0x18) = (s32)MU32((void *)(u32)M32(entity, 0x30), 0x4);
    M32(entity, 0x1C) = (s32)((s32)g_06098570 * 8) + 6;

    scene_select_player_parameter();
    sound_queue_push((u32)M32(entity, 0x1C));

    TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
}

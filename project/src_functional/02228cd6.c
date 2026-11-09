/* player_tick_grab_pull_x [0x02228cd6]: Paired actor moves in X; loop snaps grabbed animation and state. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228cd6 (actor FSM step) - run animation_update, then home a paired object toward the
 * actor on the X axis. obj = g_060995B0[entity->unk4B]->unk8. A direction step (+/-0x1000)
 * is chosen from obj->unk26 (==0x4000 -> -0x1000 else +0x1000). If entity->unk54 bit1 is set
 * (grabbed/locked): snap - anim 0x5C=0xDC, state 0x43=0x18, obj->unk8 = entity->unk8 + step.
 * Otherwise ease obj->unk8 halfway toward (entity->unk8 + step).
 * fidelity: HIGH - obj->unk8 / entity->unk8 long, obj->unk26 / entity->unk54 word (unsigned
 * test/compare), unk5C word, unk43 byte, unk4B byte index; shar = signed >>1. */
extern s32   actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern void *g_actor_slot_tasks[];   /* per-slot paired-object table, indexed by entity->unk4B */

void player_tick_grab_pull_x(void *a0, void *entity) {
    void *obj = TASK_PTR(g_actor_slot_tasks[ACTOR_M8(entity, paired_slot)], work);
    s32 step;
    actor_anim_frame_update(a0, entity);
    if (ACTOR_U16(entity, animation_events) & 2) {
        ACTOR_WORK(entity)->animation_requested = 0xDC;
        ACTOR_WORK(entity)->state_handler  = 0x18;
        step = (MU16(obj, 0x26) == 0x4000) ? -0x1000 : 0x1000;
        M32(obj, 0x8) = step + ACTOR_WORK(entity)->position_x;
    } else {
        s32 base = M32(obj, 0x8);
        step = (MU16(obj, 0x26) == 0x4000) ? -0x1000 : 0x1000;
        M32(obj, 0x8) = base + (((step + ACTOR_WORK(entity)->position_x) - base) >> 1);
    }
}

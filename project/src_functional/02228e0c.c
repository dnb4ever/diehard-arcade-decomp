/* actor_step_grab_follow [0x02228e0c]: Existing map corroborated by source: sibling of func_02228e60 (HIDDEN by fn.extent over-run of 0x228d9c; recovered via find_entry.py). animation_update; if unk54&2 set unk43=0x13; ease paired obj g_060995B0[unk4B]->unk8 pos (unk8/unk10) halfway toward entity unk8+unkC4 / unk10+unkC8 */
#include "dh_work.h"
#include "dh.h"
/* 0x02228e0c (actor FSM step) - sibling of func_02228e60. Run animation_update(a0, entity); if the
 * contact flag unk54 bit1 is set, request state unk43 = 0x13; then ease a paired object
 * (obj = g_060995B0[entity->unk4B]->unk8) toward the actor's target: obj->unk8 toward
 * entity->unk8 + unkC4 and obj->unk10 toward entity->unk10 + unkC8 (halfway each frame).
 * NOTE: this function was hidden from the unit splitter - fn.extent(0x228d9c) over-runs its rts and
 * swallows this body; its real entry 0x228e0c is recovered via tools/decomp/find_entry.py.
 * fidelity: HIGH - all position fields long; unk54 word (unsigned bit test); unk43 byte;
 * shar = signed >>1. Computed branch via braf_rewrite.py. */
extern void  actor_anim_frame_update(void *a0, void *entity);  /* animation_update */
extern void *g_actor_slot_tasks[];

void actor_step_grab_follow(void *a0, void *entity) {
    void *obj;

    actor_anim_frame_update(a0, entity);
    if (ACTOR_U16(entity, animation_events) & 2)
        ACTOR_WORK(entity)->state_handler = 0x13;
    obj = TASK_PTR(g_actor_slot_tasks[ACTOR_M8(entity, paired_slot)], work);
    M32(obj, 0x8)  += ((ACTOR_WORK(entity)->position_x  + M32(entity, 0xC4)) - M32(obj, 0x8))  >> 1;
    M32(obj, 0x10) += ((ACTOR_WORK(entity)->position_z + M32(entity, 0xC8)) - M32(obj, 0x10)) >> 1;
}

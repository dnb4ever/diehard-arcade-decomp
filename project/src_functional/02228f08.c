#include "dh_work.h"
#include "dh.h"
/* 0x0228f08 (actor_anim_state_dispatch) - per-actor animation-phase handler (task=r4, entity=r5).
 * Bails if the actor is busy (unk4B != 0). Otherwise switches on the state selector unk88 and
 * re-arms the actor's animation/request fields, calling animation_update(task, entity) on the
 * transitions that need a fresh frame.
 * fidelity: HIGH - animation_update fp resolved via ptr_resolve (0x228f24 + 0x4d08 = 0x22dc2c);
 * bsrf tails resolved to func_02227704 (0x227704) and func_0222adaa (0x22adaa). unk4B/43/67/34/4A
 * byte, unk52 word, unk88/84 long. */
extern void actor_resolve_facing(void *entity);            /* actor_facing_resolve */
extern void actor_dispatch_pose_and_type(void *entity);            /* actor_pose_dispatch  */
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update    */

void actor_anim_state_dispatch(void *task, void *entity) {
    if (ACTOR_M8(entity, paired_slot) != 0)
        return;
    switch (ACTOR_WORK(entity)->action_state) {
    case 59:
    case 60:
    case 63:
        ACTOR_WORK(entity)->action_category = 1;
        ACTOR_WORK(entity)->action_state = 0;
        ACTOR_M16(entity, interaction_latch) = 0;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
        break;
    case 64:
    case 65:
        actor_resolve_facing(entity);
        actor_anim_frame_update(task, entity);
        ACTOR_WORK(entity)->action_category = 1;
        ACTOR_WORK(entity)->action_state = 0;
        ACTOR_M16(entity, interaction_latch) = 0;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
        break;
    case 62:
        ACTOR_WORK(entity)->state_handler = 17;
        ACTOR_M16(entity, interaction_latch) = 0;
        actor_anim_frame_update(task, entity);
        break;
    case 74:
        ACTOR_WORK(entity)->state_handler = 21;
        ACTOR_M16(entity, interaction_latch) = 0;
        actor_anim_frame_update(task, entity);
        break;
    case 68:
        ACTOR_M8(entity, paired_slot) = M8(entity, 0x67);
        actor_anim_frame_update(task, entity);
        ACTOR_WORK(entity)->state_handler = 24;
        ACTOR_WORK(entity)->action_state = 0;
        break;
    case 73:
        ACTOR_M8(entity, hit_source_slot) = ACTOR_WORK(entity)->slot_id;
        actor_dispatch_pose_and_type(entity);
        ACTOR_M16(entity, interaction_latch) = 0;
        break;
    default:
        break;
    }
}

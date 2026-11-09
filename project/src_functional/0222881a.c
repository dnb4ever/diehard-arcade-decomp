/* player_tick_event_to_state6 [0x0222881a]: Event bit 0 requests handler 6 then movement/animation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222881a (actor FSM step) - if entity->unk54 bit0 is set, set state 0x43=6; always run
 * func_0222c220(entity), then tail-call animation_update(a0,entity).
 * fidelity: HIGH - unk54 word (unsigned bit test), unk43 byte; computed call/tail resolved
 * via braf_rewrite.py (bsrf 0x39e4 -> func_0222c220, braf 0x53e2 -> animation_update). */
extern s32  player_try_special_move(void *entity);
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */

void player_tick_event_to_state6(void *a0, void *entity) {
    if (ACTOR_U16(entity, animation_events) & 1)
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_EVENT_FOLLOWUP;
    player_try_special_move(entity);
    actor_anim_frame_update(a0, entity);
}

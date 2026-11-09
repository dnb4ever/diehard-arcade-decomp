/* player_tick_event_to_state9 [0x02228738]: Animation event bit 0 requests handler 9 before movement/animation. */
#include "dh_work.h"
#include "dh.h"
/* 0x0228738 (func_02228738) - actor state handler (task=r4, entity=r5). If this frame's flag
 * unk54 & 1 is set, request state unk43 = 9; then run func_0222c220(entity) and tail
 * animation_update(task, entity).
 * fidelity: HIGH - bsrf func_0222c220 (0x22c220), braf animation_update actor_anim_frame_update (0x22dc2c).
 * unk54 word, unk43 byte. (entry recovered past leading pool contamination at 0x228730.) */
extern s32  player_try_special_move(void *entity);
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update */

void player_tick_event_to_state9(void *task, void *entity) {
    if (ACTOR_U16(entity, animation_events) & 1)
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_RESPONSE;
    player_try_special_move(entity);
    actor_anim_frame_update(task, entity);
}

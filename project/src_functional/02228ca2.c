/* player_tick_loop_to_idle [0x02228ca2]: Animation loop clears latch/pending and restores action category 1 and handler 1. */
#include "dh_work.h"
#include "dh.h"
/* 0x0228ca2 (func_02228ca2) - actor state handler (task=r4, entity=r5). Runs
 * animation_update(task, entity) first; then, if this frame's loop flag unk54 & 2 is set, resets
 * the actor to the idle base: unk84 = 1, unk88 = 0, unk52 = 0, request unk43 = 1, clear unk48.
 * fidelity: HIGH - bsrf animation_update actor_anim_frame_update (0x22dc2c). unk54 word, unk84/88 long,
 * unk52 word, unk43/48 byte. */
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update */

void player_tick_loop_to_idle(void *task, void *entity) {
    actor_anim_frame_update(task, entity);
    if (ACTOR_U16(entity, animation_events) & 2) {
        ACTOR_WORK(entity)->action_category = 1;
        ACTOR_WORK(entity)->action_state = 0;
        ACTOR_M16(entity, interaction_latch) = 0;
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_IDLE;
        ACTOR_M8(entity, action_pending) = 0;
    }
}

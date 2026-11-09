/* player_tick_join_commit [0x02229bb0]: Animation update then tail-call into player join commit FSM. */
#include "dh.h"
/* 0x0229bb0 (func_02229bb0) - actor state handler (task=r4, entity=r5): run
 * animation_update(task, entity) then tail into func_02229c00(task, entity).
 * fidelity: HIGH - bsrf animation_update actor_anim_frame_update (0x22dc2c); the function restores its frame
 * and jumps to the adjacent handler 0x229c00 (rendered as a tail call). */
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update */
extern void player_join_commit_fsm(void *task, void *entity);

void player_tick_join_commit(void *task, void *entity) {
    actor_anim_frame_update(task, entity);
    player_join_commit_fsm(task, entity);
}

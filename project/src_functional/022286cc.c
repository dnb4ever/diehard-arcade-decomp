/* player_tick_grab_commands [0x022286cc]: Runs grab/throw command handler then shared animation update. */
#include "dh.h"
/* 0x02286cc (func_022286cc) - actor state handler (task=r4, entity=r5): run func_0222c39c(entity)
 * then tail animation_update(task, entity).
 * fidelity: HIGH - bsrf func_0222c39c (0x22c39c), braf animation_update actor_anim_frame_update (0x22dc2c);
 * both arguments round-tripped through the stack frame. */
extern void player_handle_grab_throw_commands(void *entity);
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update */

void player_tick_grab_commands(void *task, void *entity) {
    player_handle_grab_throw_commands(entity);
    actor_anim_frame_update(task, entity);
}

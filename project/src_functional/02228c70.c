/* player_tick_timed_hit_reaction [0x02228c70]: Frame reaches actor action_ticks then loads hit reaction and requests handler 14. */
#include "dh_work.h"
#include "dh.h"
/* 0x0228c70 (func_02228c70) - actor state handler (task=r4, entity=r5). When the animation frame
 * counter unk58 has reached its limit unk90 it runs func_0223a740(entity) and requests state
 * unk43 = 14; then tails animation_update(task, entity).
 * fidelity: HIGH - bsrf resolved func_0223a740 (0x23a740), braf animation_update actor_anim_frame_update
 * (0x22dc2c). unk58 word (unsigned), unk90 long, unk43 byte. */
extern void actor_state_gen_hitreact(void *entity);
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update */

void player_tick_timed_hit_reaction(void *task, void *entity) {
    if ((s32)ACTOR_U16(entity, animation_frame) >= ACTOR_M32(entity, action_ticks)) {
        actor_state_gen_hitreact(entity);
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_HIT_REACTION;
    }
    actor_anim_frame_update(task, entity);
}

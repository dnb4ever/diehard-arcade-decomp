/* player_tick_timed_action_finish [0x02229854]: After animation frame limit tail-calls 0223ac9c completion helper. */
#include "dh_work.h"
#include "dh.h"
/* 0x02229854 (actor FSM step) - run animation_update(a0,entity). If entity->unk48 is set,
 * tail-call func_0222adaa(entity) and return. Otherwise, once the anim frame counter unk58
 * reaches or passes its target unk90, tail-call func_0223ac9c(entity).
 * fidelity: HIGH - unk48 byte, unk58 word (unsigned, promoted) vs unk90 long (signed cmp/ge).
 * Computed branches via braf_rewrite.py. */
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern void actor_dispatch_pose_and_type(void *entity);
extern void actor_spawn_special_debris(void *entity);

void player_tick_timed_action_finish(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
        return;
    }
    if ((s32)ACTOR_U16(entity, animation_frame) >= ACTOR_M32(entity, action_ticks))
        actor_spawn_special_debris(entity);
}

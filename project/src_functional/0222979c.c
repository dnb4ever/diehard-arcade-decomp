/* player_tick_timed_release [0x0222979c]: Frame limit invokes release generator then requests handler 0x22. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222979c (actor FSM step) - run animation_update(a0,entity). If entity->unk48 is set,
 * tail-call func_0222adaa(entity) and return. Otherwise, when unk58 reaches unk90, run
 * func_0223a3a2(entity) and set state 0x43=0x22.
 * fidelity: HIGH - unk48 byte, unk58 word (unsigned) vs unk90 long, unk43 byte.
 * Computed branches via braf_rewrite.py. */
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern void actor_dispatch_pose_and_type(void *entity);
extern void actor_state_release_gen(void *entity);

void player_tick_timed_release(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
        return;
    }
    if (ACTOR_U16(entity, animation_frame) == ACTOR_M32(entity, action_ticks)) {
        actor_state_release_gen(entity);
        ACTOR_WORK(entity)->state_handler = 0x22;
    }
}

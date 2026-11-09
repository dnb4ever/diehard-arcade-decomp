/* player_tick_timed_effect_response [0x02229816]: Frame limit uses 0223b1c0 response plus 0x21; pending-hit short circuit. */
#include "dh_work.h"
#include "dh.h"
/* 0x02229816 (actor FSM step) - run animation_update(a0,entity). If entity->unk48 is set,
 * tail-call func_0222adaa(entity) and return. Otherwise, when unk58 reaches unk90, advance
 * state: unk43 = func_0223b1c0(entity) + 0x21.
 * fidelity: HIGH - unk48 byte, unk58 word (unsigned) vs unk90 long, unk43 byte.
 * Computed branches via braf_rewrite.py. */
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern void actor_dispatch_pose_and_type(void *entity);
extern s32  actor_begin_hold_effect(void *entity);

void player_tick_timed_effect_response(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
        return;
    }
    if (ACTOR_U16(entity, animation_frame) == ACTOR_M32(entity, action_ticks))
        ACTOR_WORK(entity)->state_handler = (s8)(actor_begin_hold_effect(entity) + 0x21);
}

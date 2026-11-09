/* player_tick_timed_action_response [0x02229644]: Frame limit chooses response handler from actor selector plus 0x21; pending-hit path first. */
#include "dh_work.h"
#include "dh.h"
/* 0x02229644 (actor FSM step) - run animation_update(a0,entity). If entity->unk48 (pending
 * flag) is set: run func_0223a394(entity) then tail-call func_0222adaa(entity) and return.
 * Otherwise, when the anim frame counter unk58 reaches its target unk90, advance state:
 * unk43 = func_02239fc6(entity) + 0x21.
 * fidelity: HIGH - unk48 byte, unk58 word (unsigned) vs unk90 long, unk43 byte.
 * Body is identical to func_02229714 / func_02229758 (sibling handlers, distinct ROM copies).
 * Computed branches via braf_rewrite.py. */
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern void effect_mark_actor_slot_ready(void *entity);
extern void actor_dispatch_pose_and_type(void *entity);
extern s32  actor_apply_effect_action(void *entity);

void player_tick_timed_action_response(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0) {
        effect_mark_actor_slot_ready(entity);
        actor_dispatch_pose_and_type(entity);
        return;
    }
    if (ACTOR_U16(entity, animation_frame) == ACTOR_M32(entity, action_ticks))
        ACTOR_WORK(entity)->state_handler = (s8)(actor_apply_effect_action(entity) + 0x21);
}

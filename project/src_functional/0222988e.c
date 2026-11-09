/* player_tick_loop_special_pose [0x0222988e]: Pending hit dispatch else loop loads special pose and handler 13. */
#include "dh_work.h"
#include "dh.h"
/* 0x022988e (func_0222988e) - actor state handler (task=r4, entity=r5). If the actor is flagged
 * (unk48 != 0) run func_0222adaa(entity) (pose_dispatch); else if this frame's loop flag unk54 & 2
 * is set run func_0223a6dc(entity) (pose_load) and request state unk43 = 13; always tail
 * animation_update(task, entity).
 * fidelity: HIGH - bsrf/braf resolved: func_0222adaa (0x22adaa), func_0223a6dc (0x23a6dc),
 * animation_update actor_anim_frame_update (0x22dc2c). unk48/43 byte, unk54 word. */
extern void actor_dispatch_pose_and_type(void *entity);              /* actor_pose_dispatch */
extern void actor_load_pose_animation(void *entity);              /* actor_pose_load     */
extern void actor_anim_frame_update(void *task, void *entity);  /* animation_update    */

void player_tick_loop_special_pose(void *task, void *entity) {
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
    } else if (ACTOR_U16(entity, animation_events) & 2) {
        actor_load_pose_animation(entity);
        ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
    }
    actor_anim_frame_update(task, entity);
}

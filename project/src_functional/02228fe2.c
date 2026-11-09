/* player_tick_pose_loop_to_idle [0x02228fe2]: Pending hit dispatch else loop restores facing, idle pose and handler 1. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228fe2 (actor FSM step) - if entity->unk48 is set, run func_0222adaa(entity). Otherwise,
 * when entity->unk54 bit1 is set, run func_02227704(entity), set unk84=1, unk88=0, load a new
 * pose (unk5C = g_06096C74[unk34]) and state 0x43=1. Always tail-call animation_update(a0,entity).
 * fidelity: HIGH - unk48/43 byte, unk54 word (unsigned bit test), unk5C word, unk84/88 long,
 * unk34 byte index. Computed branches via braf_rewrite.py. */
extern void actor_dispatch_pose_and_type(void *entity);
extern void actor_resolve_facing(void *entity);
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern s32  g_06096C74[];

void player_tick_pose_loop_to_idle(void *a0, void *entity) {
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
    } else if (ACTOR_U16(entity, animation_events) & 2) {
        actor_resolve_facing(entity);
        ACTOR_WORK(entity)->action_category = 1;
        ACTOR_WORK(entity)->action_state = 0;
        ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[ACTOR_WORK(entity)->slot_id];
        ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_IDLE;
    }
    actor_anim_frame_update(a0, entity);
}

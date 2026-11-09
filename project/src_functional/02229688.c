/* player_tick_animation_idle_reset [0x02229688]: Animation then pending-hit dispatch or loop-driven idle/latch reset. */
#include "dh_work.h"
#include "dh.h"
/* 0x02229688 (actor FSM step) - run animation_update(a0,entity). If entity->unk48 is set,
 * tail-call func_0222adaa(entity) and return. Otherwise, when entity->unk54 bit1 is set,
 * set unk84=1, unk88=0, unk52=0, load pose unk5C=g_06096C74[unk34] and state 0x43=1.
 * fidelity: HIGH - unk48/43 byte, unk54 word (unsigned bit test), unk52/5C word, unk84/88
 * long, unk34 byte index. Computed branches via braf_rewrite.py. */
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern void actor_dispatch_pose_and_type(void *entity);
extern s32  g_06096C74[];

void player_tick_animation_idle_reset(void *a0, void *entity) {
    actor_anim_frame_update(a0, entity);
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
        return;
    }
    if (ACTOR_U16(entity, animation_events) & 2) {
        ACTOR_WORK(entity)->action_category = 1;
        ACTOR_WORK(entity)->action_state = 0;
        ACTOR_M16(entity, interaction_latch) = 0;
        ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[ACTOR_WORK(entity)->slot_id];
        ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_IDLE;
    }
}

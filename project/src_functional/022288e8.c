/* player_tick_landing_recovery [0x022288e8]: Movement/hit checks precede loop-driven special/idle/death transition. */
#include "dh_work.h"
#include "dh.h"
/* 0x022288e8 (actor FSM step) - run func_0222c220(entity); if it returns nonzero, do nothing
 * more this frame. Else if entity->unk48 is set, run func_0222adaa(entity). Else when
 * entity->unk54 bit1 is set: if unk56 != 0, branch on the 0x02000000 bit of unk8C -
 *   set: run func_0223a6dc(entity), state 0x43=0xD;
 *   clear: clear unk48, set unk84=1/unk88=0, load pose unk5C=g_06096C74[unk34], state 0x43=1;
 * if unk56 == 0: set unk5C=0x13D, state 0x43=0x1D, unk84=0x20000000.
 * Always tail-call animation_update(a0,entity).
 * fidelity: HIGH - unk48/43 byte, unk54/56 word, unk8C/84 long (0x02000000 bit test), unk5C
 * word, unk34 byte index. Computed branches via braf_rewrite.py. */
extern void actor_dispatch_pose_and_type(void *entity);
extern s32  player_try_special_move(void *entity);
extern void actor_load_pose_animation(void *entity);
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern s32  g_06096C74[];

void player_tick_landing_recovery(void *a0, void *entity) {
    if (player_try_special_move(entity) != 0) {
        /* trigger consumed; fall through to animation_update */
    } else if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
    } else if (ACTOR_U16(entity, animation_events) & 2) {
        if (ACTOR_M16(entity, health) != 0) {
            if (ACTOR_M32(entity, interaction_flags) & 0x02000000) {
                actor_load_pose_animation(entity);
                ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_SPECIAL_POSE;
            } else {
                ACTOR_M8(entity, action_pending)  = 0;
                ACTOR_WORK(entity)->action_category = 1;
                ACTOR_WORK(entity)->action_state = 0;
                ACTOR_WORK(entity)->animation_requested = (s16)g_06096C74[ACTOR_WORK(entity)->slot_id];
                ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_IDLE;
            }
        } else {
            ACTOR_WORK(entity)->animation_requested = 0x13D;
            ACTOR_WORK(entity)->state_handler  = 0x1D;
            ACTOR_WORK(entity)->action_category = 0x20000000;
        }
    }
    actor_anim_frame_update(a0, entity);
}

/* actor_step_launch [0x022289b8]: Existing map corroborated by source: on animation_update trigger, arms a launch arc (unkC4/C8/78, anim 0x143/0x145 by facing) */
#include "dh_work.h"
#include "dh.h"
/* 0x022289b8 (actor FSM step) - run animation_update(a0,entity); when it signals a trigger
 * frame (nonzero return), arm a launch: state 0x43=8, clear 0x48, copy Y (unkC) into unkC4,
 * set unkC8=0x3000, 0x41=1, 0x88=7, and choose a launch arc from the current horizontal
 * velocity sign (unk78): moving right(>0) -> go left (unk78=-0x2000, anim 0x143), else go
 * right (unk78=0x2000, anim 0x145); the anim id goes in unk5C.
 * fidelity: HIGH - widths from the loads (0x41/0x43/0x48 byte, 0x5C word, unkC/C4/C8/78/88
 * long, unk78 signed cmp/pl); animation_update 2-arg form, its return value used. */
extern s32 actor_anim_frame_update(void *a0, void *entity);   /* animation_update */

void actor_step_launch(void *a0, void *entity) {
    if (actor_anim_frame_update(a0, entity) != 0) {
        ACTOR_WORK(entity)->state_handler  = 8;
        ACTOR_M8(entity, action_pending)  = 0;
        M32(entity, 0xC4) = ACTOR_WORK(entity)->position_y;
        M32(entity, 0xC8) = 0x3000;
        ACTOR_WORK(entity)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_WORK(entity)->action_state = 7;
        if (M32(entity, 0x78) > 0) {
            M32(entity, 0x78) = -0x2000;
            ACTOR_WORK(entity)->animation_requested = 0x143;
        } else {
            M32(entity, 0x78) = 0x2000;
            ACTOR_WORK(entity)->animation_requested = 0x145;
        }
    }
}

/* actor_step_gravity [0x02228a0e]: Existing map corroborated by source: gravity/landing arc on unkC4/C8/7C; on land resets slot g_06096C74[unk34] + picks landing anim */
#include "dh_work.h"
#include "dh.h"
/* 0x02228a0e (actor FSM step) - gravity/landing handler. Integrate a falling arc:
 * unkC8 -= 0x400; unkC4 += unkC8; unk7C = unkC4 + unkC8. If entity->unk48 is set, run
 * func_0222adaa(entity). Otherwise, once unk7C drops below -0xC000 (landed), clear the
 * per-type slot g_06096C74[unk34], zero unk78/unk7C/unkC, set state 0x43=5 / 0x41=1 and
 * pick a landing anim into unk5C from unk5A (==0x143 -> 0x132 else 0x12F). Always tail-call
 * animation_update(a0,entity).
 * fidelity: HIGH - unkC4/C8/7C/78/unkC long, unk5A/5C word, unk43/41 byte, unk34 byte index;
 * the < 0xFFFF4000 test is signed (cmp/ge). Computed branches via braf_rewrite.py. */
extern s32  actor_dispatch_pose_and_type(void *entity);
extern s32  actor_anim_frame_update(void *a0, void *entity);   /* animation_update */
extern s32  g_06096C74[];

void actor_step_gravity(void *a0, void *entity) {
    M32(entity, 0xC8) -= 0x400;
    M32(entity, 0xC4) += M32(entity, 0xC8);
    M32(entity, 0x7C)  = M32(entity, 0xC4) + M32(entity, 0xC8);
    if (ACTOR_M8(entity, action_pending) != 0) {
        actor_dispatch_pose_and_type(entity);
    } else if (M32(entity, 0x7C) < (s32)0xFFFF4000) {
        g_06096C74[ACTOR_WORK(entity)->slot_id] = 0;
        M32(entity, 0x78) = 0;
        M32(entity, 0x7C) = 0;
        ACTOR_WORK(entity)->position_y  = 0;
        ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_LANDED;
        ACTOR_WORK(entity)->animation_control  = DH_ANIMATION_CONTROL_RESTART;
        ACTOR_WORK(entity)->animation_requested = (ACTOR_U16(entity, animation_current) == 0x143) ? 0x132 : 0x12F;
    }
    actor_anim_frame_update(a0, entity);
}

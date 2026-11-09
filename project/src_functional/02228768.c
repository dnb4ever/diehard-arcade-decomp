/* player_tick_fall_settle [0x02228768]: Integrates +c4/+c8 gravity and lands into handler 5 unless hit interrupts. */
#include "dh_work.h"
#include "dh.h"
/* 0x0228768 (func_02228768) - actor fall/settle state handler (task=r4, entity=r5). Integrates a
 * downward velocity accumulator: unkC8 -= 0x400 (gravity); unkC4 += unkC8 (advance); unk7C =
 * unkC4 + unkC8. Then func_0222c220(entity): if it reports done (!=0) just tail animation_update.
 * Else if flagged (unk48 != 0) run func_0222adaa(entity) (pose_dispatch); else if the actor has
 * fallen past the floor (unk7C < (s32)0xFFFF4000): land it - clear g_06096C74[unk34], unk78, unk7C,
 * unkC8, unkC; request unk43 = 5; unk41 = 1; and step the landing frame unk5C (-1 if unk88 == 7,
 * else +1). Always tail animation_update(task, entity).
 * fidelity: HIGH - bsrf func_0222c220 (0x22c220), func_0222adaa (0x22adaa), braf animation_update
 * actor_anim_frame_update (0x22dc2c). unk34/41/43/48 byte, unk5C word, unkC/78/7C/88/C4/C8 long;
 * g_06096C74 s32[]. */
extern s32  player_try_special_move(void *entity);
extern void actor_dispatch_pose_and_type(void *entity);             /* actor_pose_dispatch */
extern void actor_anim_frame_update(void *task, void *entity); /* animation_update    */
extern s32  g_06096C74[];

void player_tick_fall_settle(void *task, void *entity) {
    M32(entity, 0xC8) = M32(entity, 0xC8) - 0x400;
    M32(entity, 0xC4) = M32(entity, 0xC4) + M32(entity, 0xC8);
    M32(entity, 0x7C) = M32(entity, 0xC4) + M32(entity, 0xC8);
    if (player_try_special_move(entity) == 0) {
        if (ACTOR_M8(entity, action_pending) != 0) {
            actor_dispatch_pose_and_type(entity);
        } else if (M32(entity, 0x7C) < (s32)0xFFFF4000) {
            g_06096C74[(s8)ACTOR_WORK(entity)->slot_id] = 0;
            M32(entity, 0x78) = 0;
            M32(entity, 0x7C) = 0;
            M32(entity, 0xC8) = 0;
            ACTOR_WORK(entity)->position_y = 0;
            ACTOR_WORK(entity)->state_handler = DH_PLAYER_HANDLER_LANDED;
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            if (ACTOR_WORK(entity)->action_state == 7)
                ACTOR_WORK(entity)->animation_requested = (s16)(ACTOR_WORK(entity)->animation_requested - 1);
            else
                ACTOR_WORK(entity)->animation_requested = (s16)(ACTOR_WORK(entity)->animation_requested + 1);
        }
    }
    actor_anim_frame_update(task, entity);
}

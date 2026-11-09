/* evt_actor_anim_physics_step [0x0221b0a2]: Existing map corroborated by source: 2-state motion/anim step; rng jitter; func_02154fc */
#include "dh_work.h"
#include "dh.h"
/* 0x0221b0a2  evt_actor_anim_physics_step - scripted-event actor step (task FSM, 2
 * states). The separate handler that followed the braf in the merged extent at
 * 0x0221b080. arg0 = task; task->unk8 = entity; task->unk10 = state.
 *   state 0: seed the entity's motion fields:
 *       entity->unk8  = 0x00030000;               (position)
 *       entity->unkC  = (task->unkF != 0) ? 0x00008000 : 0x00016000;  (velocity)
 *       entity->unk10 = 0x001e2000;
 *       entity->unk58 = 14;                        (lifetime counter)
 *       entity->unk74 = rng_next() & 0x17FF;       (per-frame velocity jitter)
 *       task->unk10++;  (falls through into state 1)
 *   state 1 (per frame):
 *       entity->unkC  += entity->unk74;
 *       entity->unk8  += entity->unk70;
 *       entity->unk64 -= 0x3000;
 *       entity->unk30  = (s16)((++entity->unk60 & 7) + 0x188);   (animation frame)
 *       func_022154fc(entity, entity->unk64, 0x00050000, 1);
 *       if (--entity->unk58 == 0) task->unk0 = 0;  (terminate)
 * fidelity: HIGH. rng_next (0x210020) is nilary; func_022154fc (0x2154fc) takes four
 * args (r4..r7 all live at the call: entity, unk64, 0x50000, 1). bsrf resolved.
 * The first (unconditional) velocity store is the asm default before the unkF branch;
 * state 0 falls through to state 1. Matches m2c_fn.py --asm. */

extern s32  game_random_next(void);                               /* rng_next */
extern void sprite_draw_billboard(void *entity, s32 a, s32 b, s32 c);   /* 0x2154fc */

void evt_actor_anim_physics_step(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x8)  = 0x00030000;
        M32(entity, 0xC)  = 0x00008000;
        M32(entity, 0x10) = 0x001e2000;
        M32(entity, 0x58) = 14;
        M32(entity, 0xC) = (M8(task, 0xF) == 0) ? 0x00016000 : 0x00008000;
        M32(entity, 0x74) = game_random_next() & 0x17FF;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        /* fallthrough */

    case 1:
        M32(entity, 0xC) += M32(entity, 0x74);
        M32(entity, 0x8) += M32(entity, 0x70);
        M32(entity, 0x64) -= 0x3000;
        M32(entity, 0x60) += 1;
        M16(entity, 0x30) = (s16)((M32(entity, 0x60) & 7) + 0x188);
        sprite_draw_billboard(entity, M32(entity, 0x64), 0x00050000, 1);
        M32(entity, 0x58) -= 1;
        if (M32(entity, 0x58) == 0) {
            TASK_PTR(task, handler) = 0;
        }
        break;

    default:
        break;
    }
}

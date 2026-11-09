#include "dh_work.h"
/* enemy_update_clash [0x02249bba]: Resolves clash outcomes and phase transitions unless the ready-player mask cancels the clash. */
#include "dh.h"
/* 0x02249bba  actor clash-resolve handler. ABI (r4=task, r5=entity). Always ends
 * by tail-calling animation_update (0x0222dc2c).
 *
 *   if (*(s32*)0x06098564 != 0) {           (clash globally cancelled)
 *       func_022469f2(entity); animation_update(task, entity); return;
 *   }
 *   engaged = entity[0x54] & 2;
 *   switch (entity[0xd8]) {
 *     case 0:
 *       if (engaged) switch (func_022444b6(entity)) {   (clash outcome)
 *         0: entity[0xd8]=2; entity[0x41]=1; func_02249b80(entity); break;
 *         1: break;
 *         2: entity[0xd8]=1; entity[0xe0]|=0x10000; entity[0x84]|=0x10;
 *            entity[0x5c]=entity[0x14a]; break;
 *       }
 *       break;
 *     case 1:
 *       if (engaged) {
 *         entity[0xd8]=0; entity[0x44]^=1; entity[0x26]+=0x8000;
 *         entity[0x84]=0x201; entity[0xe0]&=~0x10000; entity[0x5c]=entity[0x148];
 *         animation_update(task, entity); entity[0x54]|=2; return;
 *       }
 *       (falls through to case 2 when not engaged)
 *     case 2:
 *       if (engaged) func_02249b80(entity);
 *       break;
 *   }
 *   animation_update(task, entity);         (braf tail, resolved)
 *
 * func_022444b6 is 1-arg (reads only r4) and returns the clash outcome.
 * func_02249b80 is actor_pick_anim_variant (this unit). The case-1 not-engaged path
 * literally jumps into case 2's engaged test, modelled here as a fallthrough (safe:
 * engaged is false there, so func_02249b80 is skipped). fidelity: HIGH - hand-traced
 * vs m2c_fn --asm. */
extern s32  enemy_sweep_to_anchor(void *entity);
extern void enemy_reset_idle_or_walk(void *entity);
extern void enemy_pick_optional_animation(void *entity);                 /* 0x02249b80 */
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */

void enemy_update_clash(void *task, void *entity) {
    s32 engaged;

    if (*(s32 *)(uintptr_t)DH_READY_PLAYER_MASK_ADDR != 0) {
        enemy_reset_idle_or_walk(entity);
        actor_anim_frame_update(task, entity);
        return;
    }

    engaged = 2 & (u16)ACTOR_M16(entity, animation_events);
    switch (ACTOR_M32(entity, behavior_phase)) {
    case 0:
        if (engaged) {
            switch (enemy_sweep_to_anchor(entity)) {
            case 0:
                ACTOR_M32(entity, behavior_phase) = 2;
                ACTOR_M8(entity, animation_control) = DH_ANIMATION_CONTROL_RESTART;
                enemy_pick_optional_animation(entity);
                break;
            case 1:
                break;
            case 2:
                ACTOR_M32(entity, behavior_phase) = 1;
                ACTOR_M32(entity, state_flags) |= 0x10000;
                ACTOR_M32(entity, action_category) |= 0x10;
                ACTOR_MU16(entity, animation_requested) = ACTOR_M16(entity, approach_animation);
                break;
            }
        }
        break;
    case 1:
        if (engaged) {
            ACTOR_M32(entity, behavior_phase) = 0;
            ACTOR_M8(entity, facing_right) ^= 1;
            ACTOR_MU16(entity, angle_y) += 0x8000;
            ACTOR_M32(entity, action_category) = 0x201;
            ACTOR_M32(entity, state_flags) &= 0xFFFEFFFF;
            ACTOR_MU16(entity, animation_requested) = ACTOR_M16(entity, idle_animation);
            actor_anim_frame_update(task, entity);
            ACTOR_MU16(entity, animation_events) |= 2;
            return;
        }
        /* fallthrough */
    case 2:
        if (engaged) {
            enemy_pick_optional_animation(entity);
        }
        break;
    }
    actor_anim_frame_update(task, entity);
}

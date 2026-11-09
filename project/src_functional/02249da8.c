/* enemy_retract_task [0x02249da8]: Runs the four-phase scripted retract/despawn sequence and its render/grid continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x02249da8  actor_retract_task: 4-state despawn/retract task (installed by
 * actor_enter_despawn @0x249cda). ABI: single arg r4 = task; entity = task->unk8.
 *
 *   switch (entity[0xd4]) {
 *     case 0:
 *       actor_script_step(entity); animation_update(task, entity);
 *       if ((u16)entity[0x11e] > 4) entity[0xd4] = 1;
 *       func_022462bc(entity); func_0222eee4(entity); return;   (tail pair)
 *     case 1:
 *       actor_script_step(entity); animation_update(task, entity);  (falls to case 2)
 *     case 2:
 *       if (--entity[0xc4] + 1 > 0) {                            (c was >0)
 *         if (*(s32*)0x060987bc & 1) { func_022462bc(entity); func_0222eee4(entity); }
 *         return;
 *       }
 *       entity[0xd4] = 3; return;
 *     case 3:
 *       v = *(s32*)0x060973e0 - entity[0xf8]; *(s32*)0x060973e0 = v;
 *       if (v < 0) *(s32*)0x060973e0 = 0;
 *       release_slot_entry(entity); func_02227c22(entity); task->unk0 = 0;  (self-clear)
 *   }
 *
 * actor_script_step overwrites r5 (1-arg). actor_anim_frame_update is animation_update. The
 * case-2 guard is the asm's `tmp=c-1; c=tmp; if ((tmp+1)>0)` = "c was positive".
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void actor_script_step(void *entity);
extern void actor_anim_frame_update(void *task, void *entity);   /* animation_update */
extern void actor_draw_by_model_mode(void *entity);
extern void collision_insert_grid(void *entity);
extern void release_slot_entry(void *entity);
extern void actor_release_slot(void *entity);

void enemy_retract_task(void *task) {
    void *entity = TASK_PTR(task, work);

    switch (ACTOR_M32(entity, behavior_state)) {
    case 0:
        actor_script_step(entity);
        actor_anim_frame_update(task, entity);
        if (MU16(entity, 0x11E) > 4) {
            ACTOR_M32(entity, behavior_state) = 1;
        }
        actor_draw_by_model_mode(entity);
        collision_insert_grid(entity);
        return;
    case 1:
        actor_script_step(entity);
        actor_anim_frame_update(task, entity);
        /* fallthrough */
    case 2: {
        s32 c = M32(entity, 0xC4) - 1;
        M32(entity, 0xC4) = c;
        if (c + 1 > 0) {
            if (*(s32 *)(uintptr_t)DH_FRAME_TICK_ADDR & 1) {
                actor_draw_by_model_mode(entity);
                collision_insert_grid(entity);
            }
            return;
        }
        ACTOR_M32(entity, behavior_state) = DH_ENEMY_BEHAVIOR_DASH;
        return;
    }
    case 3: {
        s32 v = *(s32 *)(uintptr_t)0x060973E0 - M32(entity, 0xF8);
        *(s32 *)(uintptr_t)0x060973E0 = v;
        if (v < 0) {
            *(s32 *)(uintptr_t)0x060973E0 = 0;
        }
        release_slot_entry(entity);
        actor_release_slot(entity);
        TASK_S32(task, handler) = 0;
        break;
    }
    }
}

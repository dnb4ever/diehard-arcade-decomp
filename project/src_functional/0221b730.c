/* scene_tick_spawn_sound_intro [0x0221b730]: Spawn-list cursor, timed view reset, child 0601f14a and sound 0xa01400ff. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221b730 - task FSM, 4 states. arg0 = task; entity = task->unk8; state = task->unk10.
 *   state 0 (init): id = func_0222a878(); entity->unk30 = id; process_spawn_list(entity);
 *      child = func_0220feec(*(s32*)0x06099580, 0x0601f14a); sub = child->work; sub->unk8=0x35060,
 *      sub->unkC=0x60000, sub->unk10=0x1DC068, sub->unk24/26/28 = 0 (s16);
 *      if (*(s32*)0x060E0DDC != 0) func_02215f6c(0xA01400FF);
 *      entity->unk18 = *(s32*)(entity->unk30 + 4); *(s32*)0x06098580 |= 1; state = 1.
 *   state 1: entity->unk18 -= 1; when it reaches 0: func_0222a708();
 *      (*(*(s32*)0x0609882C))->unk8->unk3C = 0x10 (s32); state++.
 *   state 2: process_spawn_list(entity); *(s32*)0x06098580 |= 2; state = 5.
 *   state 5: task->unk0 = 0 (terminate).
 * fidelity: HIGH - hand-traced; 5 bsrf resolved to 0x22a878, 0x22a5d0 (twice),
 *   0x20feec, 0x215f6c, 0x22a708. */

extern s32  scene_select_player_parameter(void);                       /* 0x22a878 */
extern void process_spawn_list(void *arg0);            /* func_0222a5d0 */
extern void *task_spawn_child_work88(s32 arg0, void *handler);                   /* 0x20feec */
extern void sound_queue_push(s32 req);                 /* func_02215f6c */
extern void actor_mark_slots_for_refresh(void);                       /* 0x22a708 */

void scene_tick_spawn_sound_intro(void *task)
{
    u32 entity = (u32)(uintptr_t)TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        s32 id;
        u32 sub;

        id = scene_select_player_parameter();
        M32(entity, 0x30) = id;
        process_spawn_list((void *)(uintptr_t)entity);
        {
            void *child = task_spawn_child_work88(M32(0x06099580, 0), (void *)0x0601f14a);
            sub = (u32)(uintptr_t)TASK_PTR(child, work);
        }
        M32(sub, 0x8) = 0x00035060;
        M32(sub, 0xC) = 0x00060000;
        M32(sub, 0x10) = 0x001DC068;
        M16(sub, 0x24) = 0;
        M16(sub, 0x26) = 0;
        M16(sub, 0x28) = 0;
        if (M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0) {
            sound_queue_push(DH_SOUND_PROP_LAUNCH_COMMAND);
        }
        M32(entity, 0x18) = M32((u32)M32(entity, 0x30), 4);
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
        TASK_S8(task, state) = 1;
        break;
    }

    case 1:
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) == 0) {
            actor_mark_slots_for_refresh();
            M32((u32)M32((u32)M32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 0x10;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;

    case 2:
        process_spawn_list((void *)(uintptr_t)entity);
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 2;
        TASK_S8(task, state) = 5;
        break;

    case 5:
        TASK_PTR(task, handler) = 0;
        break;

    default:
        break;
    }
}

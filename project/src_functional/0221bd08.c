/* scene_tick_two_trigger_sequence [0x0221bd08]: Spawn, timer, view reset then 15-tick flag-2 notification and cancellation. */
#include "dh_work.h"
/* 0x0221bd08 - task FSM dispatch. task = r4; entity = *(u32*)(task+8);
 * state index = (s8)task[0x10]. Literal-pool: 0x06098580 (r12), 0x0609882c.
 *   0: entity->unk30 = scene_value_select(); process_spawn_list(entity);
 *      entity->unk18 = *(s32*)(entity->unk30 + 4); *(s32*)0x06098580 |= 1; state++.
 *   1: if (--entity->unk18 == 0) { (*0x0609882C)->unk8->unk3C = 0x10;
 *      func_0222a708(); state++. }
 *   2: process_spawn_list(entity); state++.
 *   3: if (++entity->unk18 > 0xE) { *(s32*)0x06098580 |= 2; state = 5; }
 *   5: *(u32*)task = 0.
 * fidelity: HIGH - hand-traced vs asm; bsrf targets resolved
 *   (scene_value_select 0x22a878, process_spawn_list 0x22a5d0, func_0222a708). */
#include "dh.h"

extern s32  scene_select_player_parameter(void);      /* scene_value_select */
extern void process_spawn_list(u32 entity);
extern void actor_mark_slots_for_refresh(void);      /* reset active actor slots */

void scene_tick_two_trigger_sequence(u32 task)
{
    u32 entity = (u32)TASK_S32(task, work);
    s8 state = (s8)TASK_S8(task, state);

    switch (state) {
    case 0: {
        s32 v = scene_select_player_parameter();
        SEQUENCE_M32(entity, spawn_cursor) = v;
        process_spawn_list(entity);
        SEQUENCE_M32(entity, main_ticks) = M32((u32)SEQUENCE_M32(entity, spawn_cursor), 0x4);
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
        TASK_S8(task, state) = (s8)1;
        break;
    }

    case 1: {
        s32 t = SEQUENCE_M32(entity, main_ticks) - 1;
        SEQUENCE_M32(entity, main_ticks) = t;
        if (t == 0) {
            actor_mark_slots_for_refresh();
            M32((u32)M32((u32)M32(DH_VIEW_TASK_ADDR, 0), 0x8), 0x3c) = 0x10;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }

    case 2:
        process_spawn_list(entity);
        TASK_S8(task, state) = (s8)((s8)TASK_S8(task, state) + 1);
        break;

    case 3: {
        s32 t = SEQUENCE_M32(entity, main_ticks) + 1;
        SEQUENCE_M32(entity, main_ticks) = t;
        if (t > 0xe) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 2;
            TASK_S8(task, state) = (s8)5;
        }
        break;
    }

    case 5:
        TASK_S32(task, handler) = 0;
        break;

    default:
        break;
    }
}

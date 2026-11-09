/* scene_tick_timed_child_sequence [0x0221bb40]: Six states advance spawn cursor, reset view and spawn child 021bc1c. */
#include "dh_work.h"
#include "dh.h"

/* 0x0221bb40 - task FSM, 6 states (0..5). r4 = task; entity = *(u32*)(task+8);
 * state = (s8)*(task+0x10).
 *   state 0 (init): id = func_0222a878(); entity->unk30 = id;
 *      process_spawn_list(entity); entity->unk18 = *(s32*)(id + 4); state = 1.
 *   state 1: if (--entity->unk18 == 0) { func_0222a708();
 *      (*(u32*)(*(u32*)0x0609882C + 8))->unk3C = 0x10; state++; }
 *   state 2: process_spawn_list(entity);
 *      entity->unk18 = *(s32*)(*(u32*)(entity+0x30) + 4);
 *      func_0220ff72(task, h21bc1c); state++.
 *   state 3: t = --entity->unk18; if (t < 0x19) { *(u32*)0x06098580 |= 1;
 *      state++; }
 *   state 4: t = --entity->unk18; if (t == 0) { process_spawn_list(entity);
 *      (*(u32*)(*(u32*)0x0609882C + 8))->unk3C = 0x10; state++; }
 *   state 5: *(u32*)task = 0 (terminate).
 * fidelity: HIGH - hand-traced; bsrf resolved to 0x22a878, 0x22a5d0 (x3),
 *   0x22a708, 0x20ff72; mova+add arg = 0x21bbc0 + 0x5c = 0x21bc1c (pool data). */

extern s32  scene_select_player_parameter(void);                       /* 0x22a878 */
extern void process_spawn_list(u32 entity);            /* 0x22a5d0 */
extern void actor_mark_slots_for_refresh(void);                       /* 0x22a708 */
extern void task_spawn_child_work40(u32 task, u32 data);         /* 0x20ff72 */

void scene_tick_timed_child_sequence(u32 task)
{
    u32 ent = MU32(task, TASK_WORK);

    switch ((s8)TASK_S8(task, state)) {
    case 0: {
        s32 id = scene_select_player_parameter();
        SEQUENCE_M32(ent, spawn_cursor) = id;
        process_spawn_list(ent);
        SEQUENCE_M32(ent, main_ticks) = M32(SEQUENCE_MU32(ent, spawn_cursor), 4);
        TASK_S8(task, state) = 1;
        break;
    }

    case 1: {
        s32 t = SEQUENCE_M32(ent, main_ticks) - 1;
        SEQUENCE_M32(ent, main_ticks) = t;
        if (t == 0) {
            actor_mark_slots_for_refresh();
            M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 0x10;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }

    case 2:
        process_spawn_list(ent);
        SEQUENCE_M32(ent, main_ticks) = M32(SEQUENCE_MU32(ent, spawn_cursor), 4);
        task_spawn_child_work40(task, 0x0601fc1c);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;

    case 3: {
        s32 t = SEQUENCE_M32(ent, main_ticks) - 1;
        SEQUENCE_M32(ent, main_ticks) = t;
        if (t < 0x19) {
            M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        }
        break;
    }

    case 4: {
        s32 t = SEQUENCE_M32(ent, main_ticks) - 1;
        SEQUENCE_M32(ent, main_ticks) = t;
        if (t == 0) {
            process_spawn_list(ent);
            M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 0x10;
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
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

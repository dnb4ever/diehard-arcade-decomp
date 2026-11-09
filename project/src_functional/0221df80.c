/* scene_tick_escalating_spawn_cycles [0x0221df80]: Seventeen states repeat spawn/fade/select with increasing delay. */
#include "dh_work.h"
#include "dh.h"

/* Seventeen-state staged controller sequence. States 2..14 repeat a
 * spawn / fade / select cycle while increasing the spawn delay. */
extern u32 scene_select_player_parameter(void);
extern u32 actor_spawn_fixed_slot(s32 slot);
extern void actor_select_scene_variant(u32 entity);
extern void actor_load_part_ids(u32 entity);
extern u32 task_spawn_current_work40(u32 parent, u32 handler);
extern u32 task_spawn_current_work6c(u32 parent, u32 handler);
extern u32 task_spawn_child_work88(u32 parent, u32 handler);
extern u32 game_random_next(void);

void scene_tick_escalating_spawn_cycles(u32 task)
{
    u32 entity = MU32(task, TASK_WORK);
    u32 controller = MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8);
    u32 node, actor, model, choice;
    s32 i;

    switch ((u32)TASK_S8(task, state)) {
    case 0:
        M32(controller, 0xD4) = 0x06074eb4;
        M32(MU32(DH_VIEW_TASK_ADDR, 0), 0) = 0x06013880;
        M8(MU32(DH_VIEW_TASK_ADDR, 0), 0x10) = 0;
        scene_select_player_parameter();
        for (i = 0; i < 2; i++) {
            if (!(M32(DH_ACTOR_SLOT_MASKS_ADDR, i * 4) & M32(0x06098568, 0))) continue;
            node = actor_spawn_fixed_slot(i);
            actor = MU32(node, 8);
            actor_select_scene_variant(actor);
            model = 0x06074ee4 + (s8)(i * 12);
            M32(actor, 8) = M32(model, 0);
            M32(actor, 0xC) = M32(model, 4);
            M32(actor, 0x10) = M32(model, 8);
            M16(actor, 0x5E) = i + 1;
            M16(actor, 0x56) = M16(0x06099788, i * 2);
            actor_load_part_ids(actor);
            M32(DH_ACTOR_SLOT_TASKS_ADDR, (i + 1) * 4) = node;
        }
        task_spawn_current_work40(task, 0x0600bbcc);
        task_spawn_current_work6c(task, 0x0603b828);
        task_spawn_current_work6c(task, 0x06022844);
        M32(entity, 0x18) = 60;
        TASK_S8(task, state) += 1;
        break;
    case 1:
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) != 0) break;
        choice = game_random_next() & 1;
        if (choice) M32(entity, 0x20) = 1;
        else M32(entity, 0x24) = 1;
        M32(0x0609856c, 0) = choice;
        task_spawn_current_work6c(task, 0x060227d6);
        task_spawn_child_work88(task, 0x06022630);
        M32(entity, 0x18) = 60;
        M32(entity, 0x28) = 1;
        M32(entity, 0x1C) = 1;
        TASK_S8(task, state) += 1;
        break;
    case 2:
    case 5:
    case 8:
    case 11:
    case 14:
        M32(entity, 0x1C) -= 1;
        if (M32(entity, 0x1C) == 0) {
            node = task_spawn_child_work88(task, 0x0601e452);
            M32(entity, 0x30) = node;
            M8(node, 0xF) = ((M8(0x0609856c, 3) & 1) ^ 1) + 2;
        }
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) != 0) break;
        M8(task, 0xF) = 1;
        M32(entity, 0x18) = 44;
        TASK_S8(task, state) += 1;
        break;
    case 3:
    case 6:
    case 9:
    case 12:
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) != 0) break;
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) = 12;
        M32(entity, 0x18) = 16;
        TASK_S8(task, state) += 1;
        break;
    case 4:
    case 7:
    case 10:
    case 13:
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) != 0) break;
        choice = game_random_next() & 1;
        if (choice) {
            choice += M32(entity, 0x20) != 0 ? 2 : 0;
            M32(entity, 0x20) ^= 1;
        } else {
            choice += M32(entity, 0x24) != 0 ? 2 : 0;
            M32(entity, 0x24) ^= 1;
        }
        M32(0x0609856c, 0) = choice;
        task_spawn_child_work88(task, 0x06022630);
        M32(entity, 0x18) = 40;
        M32(entity, 0x1C) = M32(entity, 0x28);
        M32(entity, 0x28) += 6;
        TASK_S8(task, state) += 1;
        break;
    case 15:
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) != 0) break;
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) = 12;
        M32(entity, 0x18) = 10;
        TASK_S8(task, state) += 1;
        break;
    case 16:
        M32(entity, 0x18) -= 1;
        if (M32(entity, 0x18) != 0) break;
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 16;
        TASK_S32(task, handler) = 0;
        break;
    default:
        break;
    }
}

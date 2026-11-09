/* scene_tick_staged_transfers [0x0221b5a0]: Alternating spawn/transfer and wait phases; arithmetic time scaling in states 4/6/8. */
#include "dh_work.h"
#include "dh.h"

/* Staged spawn/transfer sequence. Odd wait states share their timer tail;
 * states 4, 6 and 8 process the next list with an arithmetic time scaling. */
extern u32 scene_select_player_parameter(void);
extern void process_spawn_list_entry(u32 entity);
extern u32 task_spawn_child_work88(u32 parent, u32 handler);
extern void sound_queue_push(s32 request);
extern void actor_mark_slots_for_refresh(void);
extern s32 shc_shift_right_signed(s32 value, s32 count);

void scene_tick_staged_transfers(u32 task)
{
    u32 entity = MU32(task, TASK_WORK);
    u32 node, descriptor;

    switch ((u32)TASK_S8(task, state)) {
    case 0:
        SEQUENCE_M32(entity, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list_entry(entity);
        node = task_spawn_child_work88(MU32(0x06099580, 0), 0x0601f14a);
        descriptor = MU32(node, TASK_WORK);
        M32(descriptor, 8) = 0x0027a060;
        M32(descriptor, 0xC) = 0x40000;
        M32(descriptor, 0x10) = 0x00210068;
        M16(descriptor, 0x24) = 0;
        M16(descriptor, 0x26) = 0xC000;
        M16(descriptor, 0x28) = 0;
        if (M32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
            sound_queue_push((s32)DH_SOUND_PROP_LAUNCH_COMMAND);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        TASK_S8(task, state) += 1;
        break;
    case 1:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        TASK_S8(task, state) += 1;
        break;
    case 2:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), TASK_WORK), 0x3C) = 16;
        TASK_S8(task, state) += 1;
        M32(0x06096c90, 0) = 0;
        break;
    case 3:
    case 5:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), TASK_WORK), 0x3C) = 16;
        TASK_S8(task, state) += 1;
        M32(0x06096c90, 0) = TASK_S8(task, state) == 4 ? 1 : 0;
        break;
    case 4:
    case 6:
    case 8:
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = shc_shift_right_signed(M32(SEQUENCE_MU32(entity, spawn_cursor), 4),
                                               M32(0x06096c90, 0));
        TASK_S8(task, state) += 1;
        break;
    case 7:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), TASK_WORK), 0x3C) = 16;
        TASK_S8(task, state) += 1;
        break;
    case 9:
        TASK_S32(task, handler) = 0;
        break;
    default:
        break;
    }
}

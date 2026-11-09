/* scene_tick_double_branch_sequence [0x0221cad4]: Eleven-state spawn sequence; two branch passes share cursor. */
#include "dh_work.h"
#include "dh.h"

/* Eleven-state scene sequence. The two branch passes share the list cursor. */
extern u32 scene_select_player_parameter(void);
extern void process_spawn_list_entry(u32 entity);
extern void actor_mark_slots_for_refresh(void);
extern u32 task_spawn_current_work6c(u32 parent, u32 handler);
extern u32 task_spawn_current_work40(u32 parent, u32 handler);
extern u32 task_spawn_child_work88(u32 parent, u32 handler);
extern void player_update_aim_limits(s32 mode);
extern s32 shc_shift_left(s32 value, s32 count);

void scene_tick_double_branch_sequence(u32 task)
{
    u32 entity = MU32(task, TASK_WORK);
    u32 choice, selected;
    s32 timer;

    switch ((u32)TASK_S8(task, state)) {
    case 0:
        SEQUENCE_M32(entity, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list_entry(entity);
        task_spawn_current_work6c(task, 0x0603b828);
        task_spawn_current_work40(task, 0x0600bbcc);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        TASK_S8(task, state) += 1;
        break;
    case 1:
        if (SEQUENCE_M32(entity, main_ticks) == 21) M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), TASK_WORK), 0x3C) = 16;
        TASK_S8(task, state) += 1;
        break;
    case 2:
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = shc_shift_left(M32(SEQUENCE_MU32(entity, spawn_cursor), 4),
                                               M32(0x06096c90, 0));
        SEQUENCE_M32(entity, phase_ticks) = MU32(0x06098570, 0) * 8;
        TASK_S8(task, state) += 1;
        break;
    case 3:
        timer = SEQUENCE_M32(entity, phase_ticks);
        SEQUENCE_M32(entity, phase_ticks) = (u32)timer - 1;
        if (timer == 0) task_spawn_child_work88(task, 0x0601dc44);
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = shc_shift_left(M32(SEQUENCE_MU32(entity, spawn_cursor), 4),
                                               M32(0x06096c90, 0));
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 4;
        TASK_S8(task, state) += 1;
        break;
    case 4:
        timer = SEQUENCE_M32(entity, phase_ticks);
        SEQUENCE_M32(entity, phase_ticks) = (u32)timer - 1;
        if (timer == 0) {
            task_spawn_child_work88(task, 0x0601dc44);
            TASK_S8(task, state) += 1;
        }
        /* fall through */
    case 5:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        choice = MU32(0x0609856c, 0) & 7;
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), TASK_WORK), 0x3C) = 16;
        if (choice & 1) {
            M32(0x06098570, 0) += 1;
            if (M32(0x06098570, 0) > 4) M32(0x06098570, 0) = 4;
            player_update_aim_limits(0);
        } else {
            M32(0x06098570, 0) = 0;
        }
        selected = MU32(MU32(SEQUENCE_MU32(entity, spawn_cursor), 0x14), choice * 4);
        SEQUENCE_M32(entity, branch_cursor) = selected;
        SEQUENCE_M32(entity, spawn_cursor) = selected;
        M32(0x06096c90, 0) = 1;
        M32(entity, 8) = 1;
        M8(task, 0xF) = 1;
        TASK_S8(task, state) = 6;
        break;
    case 6:
    case 8:
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, spawn_cursor) = SEQUENCE_M32(entity, branch_cursor);
        SEQUENCE_M32(entity, main_ticks) = shc_shift_left(M32(SEQUENCE_MU32(entity, branch_cursor), 4),
                                               M32(0x06096c90, 0));
        M8(task, 0x12) = 1;
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
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), TASK_WORK), 0x3C) = 16;
        M32(entity, 8) = 1;
        M32(0x06096c90, 0) = 0;
        TASK_S8(task, state) = 10;
        break;
    case 10:
        TASK_S32(task, handler) = 0;
        break;
    default:
        break;
    }
}

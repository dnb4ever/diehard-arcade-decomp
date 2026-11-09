/* scene_tick_branch_drain [0x0221b8e4]: Twelve-state spawn/wait/branch/drain controller; state 5 falls through to wait. */
#include "dh_work.h"
#include "dh.h"

/* Twelve-state scene sequence: spawn, wait, select a branch, then drain it. */
extern u32 task_spawn_current_work40(u32 parent, u32 handler);
extern u32 task_spawn_current_work6c(u32 parent, u32 handler);
extern u32 task_spawn_child_work88(u32 parent, u32 handler);
extern u32 scene_select_player_parameter(void);
extern void process_spawn_list_entry(u32 entity);
extern void actor_mark_slots_for_refresh(void);
extern void stage_build_render_grid(s32 a, s32 b);
extern void vdp2_queue_attribute(s32 a, u32 src, u32 dst, s32 count);
extern void player_update_aim_limits(s32 mode);
extern s32 shc_shift_left(s32 value, s32 count);

void scene_tick_branch_drain(u32 task)
{
    u32 entity = MU32(task, TASK_WORK);
    u32 selected, choice;
    s32 timer;

    switch ((u32)TASK_S8(task, state)) {
    case 0:
        M32(entity, 0x28) = 0;
        M32(DH_SCENE_EVENT_FLAGS_ADDR, 0) |= 1;
        task_spawn_current_work40(task, 0x0600bbcc);
        SEQUENCE_M32(entity, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        TASK_S8(task, state) += 1;
        break;
    case 1:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        task_spawn_current_work6c(task, 0x0603b828);
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 16;
        TASK_S8(task, state) += 1;
        break;
    case 2:
        process_spawn_list_entry(entity);
        stage_build_render_grid(6, 8);
        vdp2_queue_attribute(14, MU32(0x22700b70, 0), 0x25f00000, 0x80);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        TASK_S8(task, state) += 1;
        break;
    case 3:
    case 8:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 16;
        TASK_S8(task, state) += 1;
        break;
    case 4:
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, main_ticks) = M32(SEQUENCE_MU32(entity, spawn_cursor), 4);
        SEQUENCE_M32(entity, phase_ticks) = MU32(0x06098570, 0) * 8;
        TASK_S8(task, state) += 1;
        break;
    case 5:
        timer = SEQUENCE_M32(entity, phase_ticks);
        SEQUENCE_M32(entity, phase_ticks) = (u32)timer - 1;
        if (timer == 0) {
            task_spawn_child_work88(task, 0x0601dc44);
            TASK_S8(task, state) += 1;
        }
        /* State 5 continues through state 6's wait on the same frame. */
        /* fall through */
    case 6:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        choice = MU32(0x0609856c, 0) & 7;
        actor_mark_slots_for_refresh();
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 16;
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
        TASK_S8(task, state) = 7;
        break;
    case 7:
    case 9:
        process_spawn_list_entry(entity);
        SEQUENCE_M32(entity, spawn_cursor) = SEQUENCE_M32(entity, branch_cursor);
        SEQUENCE_M32(entity, main_ticks) = shc_shift_left(M32(SEQUENCE_MU32(entity, branch_cursor), 4),
                                               M32(0x06096c90, 0));
        TASK_S8(task, state) += 1;
        break;
    case 10:
        SEQUENCE_M32(entity, main_ticks) -= 1;
        if (SEQUENCE_M32(entity, main_ticks) != 0) break;
        M32(MU32(MU32(DH_VIEW_TASK_ADDR, 0), 8), 0x3C) = 16;
        M32(entity, 8) = 1;
        M32(0x06096c90, 0) = 0;
        TASK_S8(task, state) = 11;
        break;
    case 11:
        TASK_S32(task, handler) = 0;
        break;
    default:
        break;
    }
}

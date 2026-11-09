/* scene_spawn_list_once [0x0221bdce]: Selects spawn list, processes it and immediately clears task handler. */
#include "dh_work.h"
#include "dh.h"

/* 0x0221bdce - task state 0 init stub. r4 = task; ent = *(u32*)(task+8).
 * ent->unk30 = func_0222a878(); process_spawn_list(ent); state = 1; *(u32*)task = 0. */
extern s32  scene_select_player_parameter(void);            /* 0x22a878 */
extern void process_spawn_list(u32 entity); /* 0x22a5d0 */

void scene_spawn_list_once(u32 task)
{
    u32 ent = MU32(task, TASK_WORK);
    s32 r0 = scene_select_player_parameter();

    SEQUENCE_M32(ent, spawn_cursor) = r0;
    process_spawn_list(ent);
    TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    TASK_S32(task, handler) = 0;
}

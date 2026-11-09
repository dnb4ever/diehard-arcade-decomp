/* scene_tick_sound_pair62 [0x0221debc]: Spawn then cue 0xa01206ff; 62-tick delay precedes 0xa01207ff. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221debc - two-state actor step (task handler, r4 = task, w = task->unk8).
 * state 0 (init): w+0x30 = func_0222a878(); process_spawn_list(w); if sound system
 *   is live (*0x060E0DDC != 0) queue SFX 0xA01206FF; w+0x1C = 62; advance state.
 * state 1 (tick-down): decrement w+0x1C; when it hits 0, queue SFX 0xA01207FF
 *   (only if *0x060E0DDC != 0) and advance state.
 * fidelity: HIGH - w (task+0x8) is loaded in the bt/s delay slot so it is available on
 *   all paths; the intermediate "mov r0,r4" before the func_0222a5d0 bsrf is overwritten
 *   by its delay slot (mov r14,r4), so process_spawn_list takes w only. */

extern s32 scene_select_player_parameter(void);        /* fetch timeline/record pointer */
extern void process_spawn_list(void *arg0);
extern void sound_queue_push(s32 req);

void scene_tick_sound_pair62(u32 task) {
    u32 w = MU32(task, TASK_WORK);
    s32 state = (s8)TASK_S8(task, state);

    switch (state) {
    case 0:
        SEQUENCE_M32(w, spawn_cursor) = scene_select_player_parameter();
        process_spawn_list((void *)(uintptr_t)w);
        if (MU32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
            sound_queue_push(0xa01206ff);
        SEQUENCE_M32(w, phase_ticks) = 62;
        TASK_S8(task, state) = (u8)((u8)TASK_S8(task, state) + 1);
        return;

    case 1: {
        s32 n = (s32)SEQUENCE_M32(w, phase_ticks) - 1;
        SEQUENCE_M32(w, phase_ticks) = n;
        if (n != 0)
            return;
        if (MU32(DH_SOUND_EFFECTS_ENABLED_ADDR, 0) != 0)
            sound_queue_push(0xa01207ff);
        TASK_S8(task, state) = (u8)((u8)TASK_S8(task, state) + 1);
        return;
    }

    default:
        return;
    }
}

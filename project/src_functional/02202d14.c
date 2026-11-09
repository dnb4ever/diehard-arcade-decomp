/* scene_countdown_task [0x02202d14]: Initializes HUD state, ticks the scene countdown and delegates the scene continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x02202d14 - scene timer/HUD task handler. task->unk8 = entity, task->unk10 =
 * state. On first entry (state 0) it brings the scene up: func_0220f990(task),
 * spawns scene_hud_task_init (0x0220340c) under this task, and arms the
 * entity->unk50 countdown to 0xf0. Every frame it pokes two VDP/HUD words at
 * fixed counter marks (0x06098734 / +0x18), decrements the countdown, latches
 * g_06098664 = 1 when it expires, then TAIL-CALLS func_0221d302(task).
 *
 * NOTE: fn.units/m2c treat this as a huge 0x6f8-byte "table-dispatch state
 * machine" (and m2c fails outright - "cannot find branch target
 * 0x20394a" - a spurious target inside the mis-merged extent, not a function).
 * That is an artifact of fn.extent MERGING several sibling functions that are
 * separated only by `braf` tail-calls (0x202d14, 0x202d7e, 0x202dda, 0x202dfa,
 * ...), plus the usual embedded-pool mis-decode. The real func_02202d14 ends at
 * its own braf tail at 0x202d7a; it has NO computed jump table. The siblings
 * past 0x202d7e are separate entries (left for their own passes).
 *
 * fidelity: HIGH - hand-traced vs capstone disasm. bsrf resolved: 0x20f990,
 * spawn 0x20fece(handler 0x20340c), tail braf -> 0x21d302. Counter test uses
 * the pre-decrement value (cmp/pl on the original). */

#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern void  input_reset_records(void *task);                    /* 0x20f990 */
extern void *spawn_obj_0x6c(void *parent, void *handler);  /* 0x20fece */
extern void  hud_install_status_task(void *task);                    /* scene_hud_task_init */
extern void  scene_tick_teardown_timer(void *task);                    /* 0x21d302 (tail call) */

void scene_countdown_task(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 c;

    if (TASK_U8(task, state) == 0) {
        input_reset_records(task);
        spawn_obj_0x6c(task, (void *)hud_install_status_task);
        M32(entity, 0x50) = 0xf0;
    }

    if (M32(entity, 0x50) == 80) {
        *(u16 *)(uintptr_t)DH_PLAYER_INPUT_RECORDS_ADDR = 0x100;
    }
    if (M32(entity, 0x50) == 70) {
        *(u16 *)(uintptr_t)(DH_PLAYER_INPUT_RECORDS_ADDR + 0x18) = 0x200;
    }

    c = M32(entity, 0x50);
    M32(entity, 0x50) = c - 1;
    if (c <= 0) {
        G32(0x06098664) = 1;
    }

    scene_tick_teardown_timer(task);   /* tail call */
}

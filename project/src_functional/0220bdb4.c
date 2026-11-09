/* camera_run_mode2_handler [0x0220bdb4]: Ticks mode-two request timing, invokes the work handler and restores its trampoline. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220bdb4 (task handler, entity-handler trampoline) - sibling of func_0220b354, mode 2.
 * task->unk8 = entity. Runs func_0220d8d0(task, 2), then temporarily installs the entity's own
 * handler (entity->unk64) as task->unk0, calls it with the task, and restores task->unk0 to
 * this trampoline so it re-runs next frame.
 * fidelity: HIGH - entity->unk64 is a function pointer (jsr); next-state pointer resolved from
 * the PIC mova anchor (0x20bddc - 0x28 = func_0220bdb4, i.e. self). */
extern void camera_tick_request_hold(void *task, s32 arg);

void camera_run_mode2_handler(void *task) {
    void *entity = TASK_PTR(task, work);
    void (*h)(void *);

    camera_tick_request_hold(task, 2);
    h = (void (*)(void *))CAMERA_MP(entity, update_handler);
    TASK_PTR(task, handler) = (void *)h;
    h(task);
    TASK_PTR(task, handler) = (void *)camera_run_mode2_handler;
}

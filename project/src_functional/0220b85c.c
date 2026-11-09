/* camera_run_mode1_handler [0x0220b85c]: Ticks mode-one request timing, invokes the work handler and restores its trampoline. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220b85c (task handler, entity-handler trampoline) - sibling of func_0220b354, mode 1.
 * task->unk8 = entity. Runs func_0220d8d0(task, 1), then temporarily installs the entity's own
 * handler (entity->unk64) as task->unk0, calls it with the task, and restores task->unk0 to
 * this trampoline so it re-runs next frame.
 * fidelity: HIGH - entity->unk64 is a function pointer (jsr); next-state pointer resolved from
 * the PIC mova anchor (0x20b884 - 0x28 = func_0220b85c, i.e. self). */
extern void camera_tick_request_hold(void *task, s32 arg);

void camera_run_mode1_handler(void *task) {
    void *entity = TASK_PTR(task, work);
    void (*h)(void *);

    camera_tick_request_hold(task, 1);
    h = (void (*)(void *))CAMERA_MP(entity, update_handler);
    TASK_PTR(task, handler) = (void *)h;
    h(task);
    TASK_PTR(task, handler) = (void *)camera_run_mode1_handler;
}

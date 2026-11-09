#include "dh_work.h"
/* task_run_work_handler [0x0220b354]: Loads work via task+8, calls work+0x64 as a task handler and restores the trampoline. */
#include "dh.h"
/* 0x0220b354 (task handler, entity-handler trampoline) - task->unk8 = entity. Runs
 * func_0220d8d0(task, 0), then temporarily installs the entity's own handler (entity->unk64)
 * as task->unk0, calls it with the task, and restores task->unk0 to this trampoline so it
 * re-runs next frame.
 * fidelity: HIGH - entity->unk64 is a function pointer (jsr); next-state pointer resolved from
 * the PIC mova anchor (0x20b37c - 0x28 = func_0220b354, i.e. self). */
extern void camera_tick_request_hold(void *task, s32 arg);

void task_run_work_handler(void *task) {
    void *entity = TASK_PTR(task, work);
    void (*h)(void *);

    camera_tick_request_hold(task, 0);
    h = (void (*)(void *))CAMERA_MP(entity, update_handler);
    TASK_PTR(task, handler) = (void *)h;
    h(task);
    TASK_PTR(task, handler) = (void *)task_run_work_handler;
}

/* model_task_draw_work [0x0224627e]: Tail-calls shared model draw with task work; no sound queue operations in body. */
#include "dh_work.h"
#include "dh.h"
/* 0x0224627e - func_0224627e: per-frame step of a sound/voice-object task.
 *   bra 0x246282 with r4 = task->unk8 in the delay slot, i.e. a tail call
 *   func_02246282(task->unk8) (draw-one-actor via the matrix stack).
 * Installed as task->unk0 by func_02246254. Split out of the former two-function
 * file 02246254.c (one function per file).
 * fidelity: HIGH - 2 insns, hand-checked vs asm 0x24627e..0x246280. */
void actor_draw_transformed(void *obj);            /* 0x02246282 */

void model_task_draw_work(void *task) {
    actor_draw_transformed(TASK_PTR(task, work));
}

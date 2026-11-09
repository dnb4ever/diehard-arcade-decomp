/* model_task_init_packed_id [0x02246254]: Combines task+0f/+12 into model word+30; installs per-frame model draw continuation. */
#include "dh_work.h"
#include "dh.h"
/* 0x02246254 - func_02246254: first-frame entry of a sound/voice-object task.
 *   obj = task->unk8;
 *   obj->unk30 (16-bit) = ((task->unkF << 8) & 0xFF00) | (u8)task->unk12;
 *   task->unk0 = 0x0604a27e (HWRAM 0x0224627e; next-frame handler, via mova+0xA);
 *   tail (bra) func_02246282(obj) - the same body func_0224627e reaches.
 * fidelity: HIGH - hand-checked vs asm 0x246254..0x24627c. */
void actor_draw_transformed(void *obj);            /* 0x02246282 */

void model_task_init_packed_id(void *task) {
    u8 *obj = TASK_PTR(task, work);
    M16(obj, 0x30) = (s16)((((s32)M8(task, 0xF) << 8) & 0xFF00) |
                           (M8(task, 0x12) & 0xFF));
    TASK_PTR(task, handler) = (void *)0x0604A27E;  /* mova at runtime: HWRAM copy of 0x0224627e */
    actor_draw_transformed(obj);
}

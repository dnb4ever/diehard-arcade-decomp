/* effect_tick_source_attachment [0x022324da]: Requires source flag, saves old position then recomputes pose from source offsets. */
#include "dh_work.h"
#include "dh.h"
/* 0x022324da (actor FSM) - "follow source while flag set" task handler. src = task+0x14 ->unk8;
 * entity = task+0x8. If the source's flag bit0 (src+0x3C) is clear, kill the task (+0x0) and
 * return. Otherwise snapshot the entity's current position into +0x4C/+0x50/+0x54, recompute
 * position as source position (+0x8/+0xC/+0x10) plus this entity's offset (+0x14/+0x18/+0x1C),
 * and tail-branch (braf) to func_0222eea2(entity).
 * fidelity: HIGH - all 32-bit; braf target resolved via braf_resolve.py (delta 0xffffc980 ->
 * 0x022eea2). */
extern void collision_queue_actor(void *entity);
void effect_tick_source_attachment(void *task) {
    void *src = MP(TASK_PTR(task, parent), 0x8);
    void *entity = TASK_PTR(task, work);
    if ((M32(src, 0x3C) & 1) == 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(entity, 0x4C) = M32(entity, 0x8);
    M32(entity, 0x50) = M32(entity, 0xC);
    M32(entity, 0x54) = M32(entity, 0x10);
    M32(entity, 0x8)  = M32(src, 0x8)  + M32(entity, 0x14);
    M32(entity, 0xC)  = M32(src, 0xC)  + M32(entity, 0x18);
    M32(entity, 0x10) = M32(src, 0x10) + M32(entity, 0x1C);
    collision_queue_actor(entity);  /* braf tail-branch */
}

/* effect_tick_twelve_frame_arc [0x0223ef5a]: Twelve-frame sprite animation integrates Y and grows draw scale from frame count. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223ef5a (actor FSM) - 12-frame scripted action (counter at entity+0x60, task node at r4).
 * Advance the counter; once it reaches 12 clear the task (+0x0) and return. While active set the
 * pose field +0x30 = (frame>>1) + 0x17C, integrate Y velocity (+0xC += +0x18), and tail-branch
 * (braf) to func_022153f6(entity, (frame<<16)|0x10000, 1).
 * fidelity: HIGH - counter 32-bit, pose +0x30 16-bit; braf target resolved via braf_resolve.py
 * (delta 0xfffd6460 -> 0x02153f6). */
extern void sprite_draw_child(void *entity, s32 a, s32 b);
void effect_tick_twelve_frame_arc(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 frame = M32(entity, 0x60) + 1;
    M32(entity, 0x60) = frame;
    frame -= 1;
    if (frame >= 0xC) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M16(entity, 0x30) = (s16)((frame >> 1) + 0x17C);
    M32(entity, 0xC) += M32(entity, 0x18);
    sprite_draw_child(entity, (frame << 16) + 0x10000, 1);  /* braf tail-branch */
}

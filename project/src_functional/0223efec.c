/* effect_tick_eight_frame_flash [0x0223efec]: Eight frame IDs from 0x188, draw scale from work+70 plus 0x20000. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223efec (actor FSM) - 8-frame scripted action (counter at entity+0x6C, task node at r4).
 * Advance the counter; once it reaches 8 clear the task (+0x0) and return. While active set pose
 * field +0x30 = frame + 0x188 and tail-branch (braf) to func_022153f6(entity, +0x70 + 0x20000, 1).
 * fidelity: HIGH - counter/+0x70 32-bit, +0x30 16-bit; braf target resolved via braf_resolve.py
 * (delta 0xfffd63d2 -> 0x02153f6). */
extern void sprite_draw_child(void *entity, s32 a, s32 b);
void effect_tick_eight_frame_flash(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 frame = M32(entity, 0x6C) + 1;
    M32(entity, 0x6C) = frame;
    frame -= 1;
    if (frame >= 8) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M16(entity, 0x30) = (s16)(frame + 0x188);
    sprite_draw_child(entity, M32(entity, 0x70) + 0x20000, 1);  /* braf tail-branch */
}

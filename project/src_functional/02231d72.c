/* projectile_tick_linear_lifetime [0x02231d72]: Signed lifetime countdown, XYZ velocity integration and sprite draw tail. */
#include "dh_work.h"
#include "dh.h"
/* 0x02231d72 (actor FSM) - lifetime-countdown mover task handler (task node at r4, entity at
 * task+0x8). Decrement the 16-bit timer at entity+0x58; if it was already <= 0, clear the task
 * (+0x0) and return. Otherwise integrate velocity into position (+0x8/+0xC/+0x10 += +0x14/+0x18/
 * +0x1C) and tail-branch (braf) to func_022154fc(entity, 0x20000, 0x10000, 1).
 * fidelity: HIGH - timer 16-bit, pos/vel 32-bit; braf target resolved via braf_resolve.py
 * (delta 0xfffe3748 -> 0x02154fc). */
extern void sprite_draw_billboard(void *entity, s32 a, s32 b, s32 c);
void projectile_tick_linear_lifetime(void *task) {
    void *entity = TASK_PTR(task, work);
    s16 t = (s16)(M16(entity, 0x58) - 1);
    M16(entity, 0x58) = t;
    if ((s16)(t + 1) <= 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(entity, 0x8)  += M32(entity, 0x14);
    M32(entity, 0xC)  += M32(entity, 0x18);
    M32(entity, 0x10) += M32(entity, 0x1C);
    sprite_draw_billboard(entity, 0x20000, 0x10000, 1);  /* braf tail-branch */
}

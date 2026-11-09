/* effect_tick_ballistic_sprite [0x0222078c]: Copies parent pose, applies gravity and spin, cancels at sixty frames or negative Y. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222078c  task update step for a ballistic sub-object (task at arg0, body at arg0+8).
 * First activation (arg0+0x10 == 0): seed position (0x8/0xC/0x10) from the template
 * arg0->unk14->unk8, stamp a frame-tag into +0x28 ((frame & 0xFF) << 8), set the +0x30 kind and
 * the +0x10 active flag. Each frame: bump the +0x32 life counter and expire (clear task->unk0)
 * past 60 frames or once the Y coordinate (+0xC) goes negative; otherwise integrate
 * +0xC += +0x18 (velocity), apply gravity (+0x18 -= 0x100), spin (+0x24 += 0x400, +0x26 += 0x300)
 * and tail-call the renderer func_022153f6(body, 0x8000, 1).
 * fidelity: HIGH - field widths and the braf tail call verified against the ROM. */
extern s32 g_frame_tick;   /* frame counter */
extern void sprite_draw_child(void *body, s32 arg1, s32 arg2);
void effect_tick_ballistic_sprite(void *task) {
    void *body = TASK_PTR(task, work);
    s16 life;
    if (TASK_S8(task, state) == 0) {
        void *tmpl = MP(TASK_PTR(task, parent), 8);
        M32(body, 0x8)  = M32(tmpl, 0x8);
        M32(body, 0xC)  = M32(tmpl, 0xC);
        M32(body, 0x10) = M32(tmpl, 0x10);
        M16(body, 0x28) = (s16)((g_frame_tick & 0xFF) << 8);
        M16(body, 0x30) = 0xA;
        TASK_S8(task, state) += 1;
    }
    life = M16(body, 0x32) + 1;
    M16(body, 0x32) = life;
    if (life > 0x3C || M32(body, 0xC) < 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(body, 0xC)  += M32(body, 0x18);
    M32(body, 0x18) -= 0x100;
    M16(body, 0x24) += 0x400;
    M16(body, 0x26) += 0x300;
    sprite_draw_child(body, 0x8000, 1);
}

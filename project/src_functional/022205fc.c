/* prop_tick_weighted_path_launch [0x022205fc]: Four-tap vertex interpolation and accumulating launch offset/rotation. */
#include "dh_work.h"
#include "dh.h"
/* 0x022205fc  projectile / animated-object physics task.
 * task (r4): +0x08 -> work struct, +0x10 = state byte.
 * work: +0x08 position accumulator, +0x0C step, +0x10, +0x24/26/28 angle words, +0x36/0x37
 *       flags, +0x60 phase, +0x64 phase-rate, +0x64..., +0x7C param, +0x64 rate.
 *   state 0: zero unk8/unk10/unk24/unk26/unk28/unk60, set unkC=0x40000, unk37=1, unk36=0; advance.
 *   every state: func_0221400c(work->unk7C, work->unk60, &work->unk8, &work->unk24); then
 *     unk60 += 0x200; unk8 += unk64; unk64 += 0x4000; unk24 -= 0x4000; tail-call func_0221b330(work).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf -> 0x21400c, braf (tail) -> 0x21b330.
 * Note: this unit's ROM extent also covers the separate function at 0x022206ae. */

extern void vertex_blend_four(s32 a, s32 b, void *c, void *d);
extern void model_draw_composite_parts(void *work);

void prop_tick_weighted_path_launch(void *task) {
    void *work = TASK_PTR(task, work);

    if (TASK_U8(task, state) == 0) {
        M32(work, 0x08) = 0;
        M32(work, 0x0C) = 0x40000;
        M32(work, 0x10) = 0;
        M16(work, 0x24) = 0;
        M16(work, 0x26) = 0;
        M16(work, 0x28) = 0;
        M32(work, 0x60) = 0;
        MU8(work, 0x37) = 1;
        MU8(work, 0x36) = 0;
        TASK_U8(task, state)++;
    }

    vertex_blend_four(M32(work, 0x7C), M32(work, 0x60), (u8 *)work + 0x08, (u8 *)work + 0x24);
    M32(work, 0x60) += 0x200;
    M32(work, 0x08) += M32(work, 0x64);
    M32(work, 0x64) += 0x4000;
    M16(work, 0x24) = (s16)(M16(work, 0x24) - 0x4000);
    model_draw_composite_parts(work);
}

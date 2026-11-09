/* prop_tick_phased_rotation [0x022206ae]: Scene-76 preset selects model, thirty-frame hold then rotating acceleration. */
#include "dh_work.h"
#include "dh.h"
/* 0x022206ae  phased swing/rotate animation task (sibling of 0x022205fc, hidden inside the same
 * ROM compilation unit; not a separate triage entry).
 * task (r4): +0x08 -> work struct, +0x10 = state byte.
 * work: +0x08, +0x0C, +0x10, +0x14 offset accumulator, +0x24/26/28 angle words, +0x30 preset,
 *       +0x32 phase frame counter.
 *   state 0: load preset from g_060755CC[*g_06098668 - 76] -> unk30; init unk8=0, unkC=0x4000,
 *            unk10=0x10000, unk24=0, unk26=0x4000, unk28=0, unk14=0x2000; advance (falls into 1).
 *   state 1: ++unk32; when it passes 30 reset it to 0 and advance; return either way.
 *   state 2: unk26 -= 0x400; unk14 += 0x600; ++unk32; advance when it passes 15; fall into 3.
 *   state 3/other: tail-call func_022154fc(work, work->unk14, work->unk14, 4).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. braf (tail) -> 0x2154fc. */

extern u8 g_scene_index[];   /* s32 counter */
extern u8 g_060755CC[];   /* s16 preset table */
extern void sprite_draw_billboard(void *work, s32 a, s32 b, s32 c);

void prop_tick_phased_rotation(void *task) {
    void *work = TASK_PTR(task, work);

    switch (TASK_U8(task, state)) {
    case 0: {
        s32 idx = M32(g_scene_index, 0) - 76;
        M16(work, 0x30) = M16(g_060755CC, idx * 2);
        M32(work, 0x08) = 0;
        M32(work, 0x0C) = 0x4000;
        M32(work, 0x10) = 0x10000;
        M16(work, 0x24) = 0;
        M16(work, 0x26) = 0x4000;
        M16(work, 0x28) = 0;
        M32(work, 0x14) = 0x2000;
        TASK_U8(task, state)++;
    }
        /* fallthrough */
    case 1: {
        s32 c = M16(work, 0x32) + 1;
        M16(work, 0x32) = (s16)c;
        if ((s16)c > 30) {
            M16(work, 0x32) = 0;
            TASK_U8(task, state)++;
        }
        return;
    }
    case 2: {
        s32 c;
        M16(work, 0x26) = (s16)(M16(work, 0x26) - 0x400);
        M32(work, 0x14) += 0x600;
        c = M16(work, 0x32) + 1;
        M16(work, 0x32) = (s16)c;
        if ((s16)c > 15)
            TASK_U8(task, state)++;
    }
        /* fallthrough */
    case 3:
    default:
        sprite_draw_billboard(work, M32(work, 0x14), M32(work, 0x14), 4);
        break;
    }
}

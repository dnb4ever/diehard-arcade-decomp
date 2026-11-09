/* prop_tick_preset_launch [0x022202ca]: Four preset model/velocity records seed launch task and animated draw. */
#include "dh_work.h"
#include "dh.h"
/* 0x022202ca  preset-driven launch/animation task (state machine).
 * task (r4): +0x08 -> work struct, +0x0F = preset index (0..3), +0x10 = state byte.
 * On entry it snapshots two global tables onto the stack: 8 bytes from g_060755A4 (used as
 * s16[], indexed by preset) and 32 bytes from g_060755AC (four 8-byte {s32,s32} records).
 *   state 0: load preset -> work->unk30 (s16 from A), work->unk8/unkC (from B), set
 *            unk10=0x20000, unk24=-0x3400, unk58=30, unk5C=preset*8; advance (falls into 1).
 *   state 1: countdown work->unk5C; advance once the pre-decrement value is <= 0.
 *   state 2: work->unk24 += 0x200; countdown work->unk58; advance when it reaches 0 (falls into 3).
 *   state 3: tail-call func_022154fc(work, 0x10000, 0x10000, 4).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf -> 0x2154fc. */

extern u8 g_060755A4[];
extern u8 g_060755AC[];
extern void sprite_draw_billboard(void *work, s32 a, s32 b, s32 c);

void prop_tick_preset_launch(void *task) {
    u8 tabA[8];
    u8 tabB[32];
    void *work;
    s32 sub;
    s32 i;

    M32(tabA, 0) = M32(g_060755A4, 0);
    M32(tabA, 4) = M32(g_060755A4, 4);
    for (i = 0; i < 32; i += 4)
        M32(tabB, i) = M32(g_060755AC, i);

    work = TASK_PTR(task, work);
    switch (TASK_U8(task, state)) {
    case 0:
        sub = M8(task, 0x0F);
        M16(work, 0x30) = M16(tabA, sub * 2);
        M32(work, 0x08) = M32(tabB, sub * 8);
        M32(work, 0x0C) = M32(tabB, sub * 8 + 4);
        M32(work, 0x10) = 0x20000;
        M16(work, 0x24) = -0x3400;
        M32(work, 0x58) = 30;
        M32(work, 0x5C) = sub * 8;
        TASK_U8(task, state)++;
        /* fallthrough */
    case 1: {
        s32 old = M32(work, 0x5C);
        M32(work, 0x5C) = old - 1;
        if (old <= 0)
            TASK_U8(task, state)++;
        break;
    }
    case 2: {
        s32 n;
        M16(work, 0x24) = (s16)(M16(work, 0x24) + 0x200);
        n = M32(work, 0x58) - 1;
        M32(work, 0x58) = n;
        if (n <= 0)
            TASK_U8(task, state)++;
    }
        /* fallthrough */
    case 3:
        sprite_draw_billboard(work, 0x10000, 0x10000, 4);
        break;
    default:
        break;
    }
}

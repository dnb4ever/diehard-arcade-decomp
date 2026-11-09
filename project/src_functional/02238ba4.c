/* effect_spark_fall_back_task [0x02238ba4]: Integrates a growing ballistic spark with negative Z drift until it falls below the floor. */
#include "dh_work.h"
#include "dh.h"
/* 0x02238ba4 - ballistic sub-object task handler (3-state machine).
 * task->unk8 = entity; task->unk10 = state byte. Gravity-arced sprite that
 * advances, falls, and is drawn via draw_child_sprite each frame.
 *   state 0: seed (unk30=0x17B, unk32=0, unk58=0x30000); -> state 1 (fallthrough).
 *   state 1: unk58 += 0x2000; unk18 -= 0x400 (gravity); pos unk8 += vel unk14;
 *            pos unkC += vel unk18; unk10 -= 0x200; when unkC < 0x500 advance
 *            state; draw_child_sprite(entity, unk58, 1).
 *   state 2: deactivate task (task->unk0 = 0).
 * fidelity: HIGH - verified vs m2c_fn.py --asm; the case-1 tail braf resolves to
 *   0x022153f6 (draw_child_sprite); r6 (3rd arg) is set to 1 in a branch delay slot
 *   so it is always 1 at the call. */
extern void sprite_draw_child(void *entity, s32 scale, s32 flags);  /* 0x022153f6 draw_child_sprite */

void effect_spark_fall_back_task(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    switch (state) {
    case 0:
        M16(ent, 0x30) = 0x17B;
        M16(ent, 0x32) = 0;
        M32(ent, 0x58) = 0x30000;
        TASK_S8(task, state)++;
        /* fallthrough */
    case 1:
        M32(ent, 0x58) += 0x2000;
        M32(ent, 0x18) -= 0x400;
        M32(ent, 0x8) += M32(ent, 0x14);
        M32(ent, 0xC) += M32(ent, 0x18);
        M32(ent, 0x10) -= 0x200;
        if (M32(ent, 0xC) < 0x500)
            TASK_S8(task, state)++;
        sprite_draw_child(ent, M32(ent, 0x58), 1);
        return;
    case 2:
        TASK_PTR(task, handler) = (void *)0;
        return;
    default:
        return;
    }
}

/* effect_flash_expand_task [0x02238cd2]: Draws a short-lived expanding flash and retires its task after the count expires. */
#include "dh_work.h"
#include "dh.h"
/* 0x02238cd2 - timed sub-object task handler (3-state machine), sibling of
 * 0x02238ba4 / 0x02238c4e. task->unk8 = entity; task->unk10 = state byte.
 *   state 0: seed (unk30=0x178, unk32=0xB, unk58=0x58000, unk5C=2, unk20=0x100);
 *            -> state 1 (fallthrough).
 *   state 1: unk58 += 0x10000; when the unk5C countdown reaches 0 advance state;
 *            draw_child_sprite(entity, unk58, 1).
 *   state 2: deactivate task (task->unk0 = 0).
 * fidelity: HIGH - verified vs m2c_fn.py --asm; case-1 tail braf -> 0x022153f6
 *   (draw_child_sprite), r6 (flags) always 1 (set in a branch delay slot). */
extern void sprite_draw_child(void *entity, s32 scale, s32 flags);  /* 0x022153f6 draw_child_sprite */

void effect_flash_expand_task(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    switch (state) {
    case 0:
        M16(ent, 0x30) = 0x178;
        M16(ent, 0x32) = 0xB;
        M32(ent, 0x58) = 0x58000;
        M32(ent, 0x5C) = 2;
        M16(ent, 0x20) = 0x100;
        TASK_S8(task, state)++;
        /* fallthrough */
    case 1:
        M32(ent, 0x58) += 0x10000;
        M32(ent, 0x5C) -= 1;
        if (M32(ent, 0x5C) == 0)
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

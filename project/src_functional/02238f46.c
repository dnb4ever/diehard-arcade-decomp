/* effect_flash_scale_task [0x02238f46]: Existing map corroborated by source: 3-state scale pop (unk34/36/38 + timer unk40); tail func_022386fc(&unk34) (m2c failed on tail bra, hand-decoded) */
#include "dh_work.h"
#include "dh.h"
/* 0x02238f46 (task handler) - 3-axis scale "pop" animation driving the scale
 * triple at ent->unk34/36/38 (words) with a frame timer ent->unk40.
 * ent = task->unk8; state selector task->unk10.
 *   state 0: scale = (0xAA,100,100); advance.
 *   state 1: scale = (0xB4,100,100); timer unk40 = 30; advance.
 *   state 2: scale -= (6,3,3) per frame; when unk40 hits 0 zero the scale and
 *            kill task (task->unk0 = 0).
 * Always tail-calls func_022386fc(&ent->unk34) to apply the scale.
 * fidelity: HIGH. m2c failed on the tail bra to func_022386fc; hand-decoded. */
extern void display_accumulate_channel(void *scale);

void effect_flash_scale_task(void *task) {
    void *ent = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M16(ent, 0x34) = 0xAA;
        M16(ent, 0x36) = 100;
        M16(ent, 0x38) = 100;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1:
        M16(ent, 0x34) = 0xB4;
        M16(ent, 0x36) = 100;
        M16(ent, 0x38) = 100;
        M32(ent, 0x40) = 30;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 2:
        M16(ent, 0x34) = (s16)(M16(ent, 0x34) - 6);
        M16(ent, 0x36) = (s16)(M16(ent, 0x36) - 3);
        M16(ent, 0x38) = (s16)(M16(ent, 0x38) - 3);
        M32(ent, 0x40) -= 1;
        if (M32(ent, 0x40) == 0) {
            M16(ent, 0x34) = 0;
            M16(ent, 0x36) = 0;
            M16(ent, 0x38) = 0;
            TASK_S32(task, handler) = 0;
        }
        break;
    }
    display_accumulate_channel((void *)((u8 *)ent + 0x34));
}

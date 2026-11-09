#include "dh_work.h"
#include "dh.h"
/* 0x02206d8c (scene81_cam_update) - a per-object 2-state animation handler in the scene-81
 * unit (immediately after scene81_off16; the two share one literal-pool region, which is why
 * find_entry/fn.extent merged them - verified by disasm and resolved branch targets). ABI
 * (task): task = r4; obj = task->unk8; index = (s8)task->unkF; state = task->unk10.
 *
 * state 0: seed the object from the index - obj->unk8 = index*0x2800 - 0x3C00 (x),
 *          obj->unkC = 0 (y), obj->unk10 = 0x10000 (z), obj->unk58 = index*2,
 *          obj->unk5C = index<<14 (orbit phase), obj->unk22 = 0xE00; -> state 1.
 * state 1: advance obj->unk58 (+1), obj->unk5C (+0x1000, orbit phase), set obj->unk30 =
 *          (obj->unk5A & 7) + 0x188, then tail-call func_022154fc(obj, 0xC000,
 *          (sin_lookup(obj->unk5C) >> 3) + 0x8000, 0) to place it on a sine orbit.
 *
 * Branches resolved via braf_resolve (bsrf 0x213190 sin_lookup; braf 0x2154fc tail).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (code 0x206d8c..0x206e2e). */

extern s32  sin_lookup(s32 angle);                               /* 0x02213190 */
extern void sprite_draw_billboard(void *obj, s32 a, s32 b, s32 c);       /* 0x02154fc */

void scene81_cam_update(void *task) {
    void *obj = TASK_PTR(task, work);

    switch (TASK_U8(task, state)) {
    case 0:
        M32(obj, 0x8)  = M8(task, 0xF) * 0x2800 - 0x3C00;
        M32(obj, 0xC)  = 0;
        M32(obj, 0x10) = 0x10000;
        M32(obj, 0x58) = M8(task, 0xF) * 2;
        M32(obj, 0x5C) = M8(task, 0xF) << 0xE;
        M16(obj, 0x22) = 0xE00;
        TASK_U8(task, state)++;
        /* fallthrough */
    case 1:
        M32(obj, 0x58) += 1;
        M32(obj, 0x5C) += 0x1000;
        M16(obj, 0x30) = (s16)((M16(obj, 0x5A) & 7) + 0x188);
        sprite_draw_billboard(obj, 0xC000, (sin_lookup(M32(obj, 0x5C)) >> 3) + 0x8000, 0);
        break;
    default:
        break;
    }
}

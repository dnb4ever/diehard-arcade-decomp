/* prop_tick_eased_panel [0x02218bec]: Scene flags trigger eased angle between saved limits; model from task-selected table. */
/* 0x02218bec - 7-state task handler for a swinging/rotating 3D prop (door or panel)
 * whose angle eases between an open and closed limit, driven by scene trigger flags.
 * task (r4); work/entity = task->unk8. State byte at task->unk10:
 *   0 (init): model id work->unk78 = table 0x060547A0[task->unk12] (s16); set facing
 *      flag work->unk74 (0 if (u16)work->unk26==0x4000 else 1); snapshot the three base
 *      angles (work->unk24/26/28 -> work->unk2A/2C/2E); set the ease limits from the
 *      initial facing: unk60 = (u16)unk26 (upper), unk64 = (u16)unk26 - 0x5000 (lower);
 *      clear counter unk7C. If task->unkF != 0 start at the lower limit closing (unk6C=
 *      unk64, unk70=1, state=4); else start at the upper limit (unk6C=unk60, state=1).
 *   1 (wait-to-close): if scene flag mask (0x060547C0[unk13].+0) is set, consume it,
 *      zero the phase, set unk70=1, advance to 2.
 *   2 (closing): unk68 += 0x100; unk6C -= sin_lookup(unk68) >> 5; clamp at unk64 and
 *      advance to 3.
 *   3: zero phase + counter, go to state 4.
 *   4 (wait-to-open): if task->unk12==1 count unk7C up and after 60 frames force state 5;
 *      if scene flag mask (0x060547C0[unk13].+2) is set, consume it and go to state 5.
 *   5 (opening): unk68 += 0x100; unk6C += sin_lookup(unk68) >> 5; clamp at unk60 and
 *      advance to 6; if the +0 mask is set again, consume it and bounce back to state 2.
 *   6: zero phase, clear unk70, back to state 1.
 * Every frame: write the accumulator's low word (unk6E) into the facing angle unk26, then
 * push a matrix frame (g_06098878 += 0x30), load the cached matrix, translate by work+8,
 * rotate by the ZYX angles at work+0x24, set model id from unk7A and submit the model;
 * when task->unk12==1 and unk70 is set, draw a second instance (reloaded matrix, translate
 * from table 0x060547A8[unk77] stride 12, rotate by the snapshot angles at work+0x2A, model
 * id 0x4ED); then pop the matrix frame.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf/braf_resolve: sin_lookup 0x213190,
 *   mtx_load_abc 0x2149aa, mtx_apply_vec_at0 0x214a50, mtx_rotate_zyx 0x2149e4,
 *   submit_model_culled 0x215a2c. s16 trigger masks are loaded sign-extended (mov.w).
 *   unk68 phase ease via sin_lookup(angle)>>5 (shar x5). */
#include "dh_band21.h"

extern s32  sin_lookup(s32 angle);            /* func_02213190 */
extern void mtx_load_abc(void);               /* func_022149aa */
extern void mtx_apply_vec_at0(u8 *obj);       /* func_02214a50 */
extern void mtx_rotate_zyx(u16 *angles);      /* func_022149e4 */
extern void submit_model_culled(u32 obj, s32 flag); /* func_02215a2c */

void prop_tick_eased_panel(u32 task)
{
    u32 work = MEMU32(task + 8);
    s32 mask;

    switch (MEM8(task + 0x10)) {
    case 0:
        MEM32(work + 0x78) = MEM16(0x060547a0 + (u32)(u8)MEM8(task + 0x12) * 2);
        if ((u16)MEM16(work + 0x26) == 0x4000)
            MEM32(work + 0x74) = 0;
        else
            MEM32(work + 0x74) = 1;
        MEM16(work + 0x2a) = MEM16(work + 0x24);
        MEM16(work + 0x2c) = MEM16(work + 0x26);
        MEM16(work + 0x2e) = MEM16(work + 0x28);
        MEM32(work + 0x60) = MEMU16(work + 0x26);
        MEM32(work + 0x64) = (s32)MEMU16(work + 0x26) - 0x5000;
        MEM32(work + 0x7c) = 0;
        if (MEM8(task + 0x0f) != 0) {
            MEM32(work + 0x6c) = MEM32(work + 0x64);
            MEM32(work + 0x70) = 1;
            MEM8(task + 0x10) = 4;
        } else {
            MEM32(work + 0x6c) = MEM32(work + 0x60);
            MEM8(task + 0x10) = 1;
        }
        break;
    case 1:
        mask = MEM16(0x060547c0 + (u32)(u8)MEM8(task + 0x13) * 4);
        if ((MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & mask) != 0) {
            MEM32(work + 0x68) = 0;
            MEM32(work + 0x70) = 1;
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~mask;
            MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        }
        break;
    case 2:
        MEM32(work + 0x68) += 0x100;
        MEM32(work + 0x6c) -= sin_lookup(MEM32(work + 0x68)) >> 5;
        if (MEM32(work + 0x6c) < MEM32(work + 0x64)) {
            MEM32(work + 0x6c) = MEM32(work + 0x64);
            MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        }
        break;
    case 3:
        MEM32(work + 0x68) = 0;
        MEM32(work + 0x7c) = 0;
        MEM8(task + 0x10) = 4;
        break;
    case 4:
        if (MEM8(task + 0x12) == 1) {
            MEM32(work + 0x7c) += 1;
            if (MEM32(work + 0x7c) > 60) {
                MEM32(work + 0x68) = 0;
                MEM8(task + 0x10) = 5;
            }
        }
        mask = MEM16(0x060547c0 + (u32)(u8)MEM8(task + 0x13) * 4 + 2);
        if ((MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & mask) != 0) {
            MEM32(work + 0x68) = 0;
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~mask;
            MEM8(task + 0x10) = 5;
        }
        break;
    case 5:
        MEM32(work + 0x68) += 0x100;
        MEM32(work + 0x6c) += sin_lookup(MEM32(work + 0x68)) >> 5;
        if (MEM32(work + 0x6c) > MEM32(work + 0x60)) {
            MEM32(work + 0x6c) = MEM32(work + 0x60);
            MEM8(task + 0x10) = (s8)(MEM8(task + 0x10) + 1);
        }
        mask = MEM16(0x060547c0 + (u32)(u8)MEM8(task + 0x13) * 4);
        if ((MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & mask) != 0) {
            MEM32(work + 0x68) = 0;
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~mask;
            MEM8(task + 0x10) = 2;
        }
        break;
    case 6:
        MEM32(work + 0x68) = 0;
        MEM32(work + 0x70) = 0;
        MEM8(task + 0x10) = 1;
        break;
    default:
        break;
    }

    MEM16(work + 0x26) = MEM16(work + 0x6e);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(work + 8));
    mtx_rotate_zyx((u16 *)(uintptr_t)(work + 0x24));
    MEM16(work + 0x30) = MEM16(work + 0x7a);
    submit_model_culled(work, 0);

    if (MEM8(task + 0x12) == 1 && MEM32(work + 0x70) != 0) {
        s32 off = (s8)((s8)MEM8(work + 0x77) * 12);
        mtx_load_abc();
        mtx_apply_vec_at0((u8 *)(uintptr_t)(0x060547a8 + off));
        mtx_rotate_zyx((u16 *)(uintptr_t)(work + 0x2a));
        MEM16(work + 0x30) = 0x4ed;
        submit_model_culled(work, 0);
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

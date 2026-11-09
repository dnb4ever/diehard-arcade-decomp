/* prop_open_close_sweep [0x0221c4ac]: Existing map corroborated by source: (u32 task) 2-panel door swing between poses; trigger mask g_060748BC[unk12]; draws 2 models in one matrix frame (src symbols_band210big.txt) */
/* 0x0221c4ac - two-piece prop open/close sweep (task handler, r4 = task, ent = task->unk8).
 * A pair of panels swings between a closed and an open pose, triggered by an event bit.
 * rec = 0x0607481C + (s8)task->unkF * 0x10 (mov.b sign-extends before shll2/shll2).
 *   0: seed targets from the record. unk60 = (rec->unkC >> 16) + (u16)unk26;
 *      unk68 = (rec->unk1C >> 16) + (u16)unk26. If task->unk13: unk64=unk60+0x3800,
 *      unk6C=unk68-0x3800 else unk64=unk60-0x3800, unk6C=unk68+0x3800. unk70=unk60,
 *      unk74=unk68, unk7C = *(0x060748BC + (s8)task->unk12*4) (trigger mask); state=1.
 *   1: if (*0x06098580 & unk7C) { clear those bits; state = task->unk13 ? 3 : 2. }
 *   2 (close): unk70-=0x400, unk74+=0x400; if (unk70 < unk64) snap to unk64/unk6C, state=4.
 *   3 (open):  unk70+=0x400, unk74-=0x400; if (unk70 > unk64) snap to unk64/unk6C, state=4.
 *   4/other: hold.
 * Always (draw tail): unk26=unk72, unk2C=unk76; push matrix (*0x06098878 += 0x30);
 *   panel A: unk30=(s16)rec->unkC; mtx_load_abc; mtx_apply_vec_at0(rec);
 *   mtx_rotate_zyx(ent+0x24); submit_model_culled(ent,0);
 *   panel B: unk30=(s16)rec->unk1C; mtx_load_abc; mtx_apply_vec_at0(rec+0x10);
 *   mtx_rotate_zyx(ent+0x2A); submit_model_culled(ent,0); pop matrix (-= 0x30).
 * ABI: r4 = task; no return.
 * fidelity: HIGH - hand-verified vs disasm. bsrf targets: mtx_load_abc 0x2149aa,
 * mtx_apply_vec_at0 0x214a50, mtx_rotate_zyx 0x2149e4, func_02215a2c 0x215a2c (x2 each).
 * CRITICAL: task->unkF (offset 15) and task->unk12 (offset 18) are mov.b loads whose
 * sign-extended values are shifted left by 4 before the table-base add, so a byte like
 * 0x86 indexes the table at -0x7a0, not +0x860. */
#include "dh_band21.h"

extern void mtx_load_abc(void);                      /* 0x022149aa */
extern void mtx_apply_vec_at0(u8 *obj);              /* 0x02214a50 */
extern void mtx_rotate_zyx(u16 *angles);             /* 0x022149e4 */
extern void model_submit_culled(u32 arg0, s32 arg1);       /* 0x02215a2c */

void prop_open_close_sweep(u32 task)
{
    u32 ent = MEMU32(task + 8);
    u32 rec = 0x0607481c + ((s32)(s8)MEM8(task + 15) << 4);

    switch (MEM8(task + 16)) {
    case 0: {
        u16 n26 = (u16)MEM16(ent + 0x26);
        MEM32(ent + 0x60) = ((u32)MEM32(rec + 0x0c) >> 16) + n26;
        MEM32(ent + 0x68) = ((u32)MEM32(rec + 0x1c) >> 16) + n26;
        if (MEM8(task + 0x13) != 0) {
            MEM32(ent + 0x64) = MEM32(ent + 0x60) + 0x3800;
            MEM32(ent + 0x6c) = MEM32(ent + 0x68) - 0x3800;
        } else {
            MEM32(ent + 0x64) = MEM32(ent + 0x60) - 0x3800;
            MEM32(ent + 0x6c) = MEM32(ent + 0x68) + 0x3800;
        }
        MEM32(ent + 0x70) = MEM32(ent + 0x60);
        MEM32(ent + 0x74) = MEM32(ent + 0x68);
        MEM32(ent + 0x7c) = MEM32(0x060748bc + ((s32)(s8)MEM8(task + 0x12) << 2));
        MEM8(task + 16) = 1;
    }
    /* fallthrough */
    case 1: {
        s32 bits = MEM32(ent + 0x7c);
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & bits) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~bits;
            MEM8(task + 16) = (MEM8(task + 0x13) != 0) ? 3 : 2;
        }
        break;
    }
    case 2:
        MEM32(ent + 0x70) -= 0x400;
        MEM32(ent + 0x74) += 0x400;
        if (MEM32(ent + 0x70) < MEM32(ent + 0x64)) {
            MEM32(ent + 0x70) = MEM32(ent + 0x64);
            MEM32(ent + 0x74) = MEM32(ent + 0x6c);
            MEM8(task + 16) = 4;
        }
        break;
    case 3:
        MEM32(ent + 0x70) += 0x400;
        MEM32(ent + 0x74) -= 0x400;
        if (MEM32(ent + 0x70) > MEM32(ent + 0x64)) {
            MEM32(ent + 0x70) = MEM32(ent + 0x64);
            MEM32(ent + 0x74) = MEM32(ent + 0x6c);
            MEM8(task + 16) = 4;
        }
        break;
    default:
        break;
    }

    MEM16(ent + 0x26) = MEM16(ent + 0x72);
    MEM16(ent + 0x2c) = MEM16(ent + 0x76);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;

    MEM16(ent + 0x30) = (s16)MEM32(rec + 0x0c);
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)rec);
    mtx_rotate_zyx((u16 *)(uintptr_t)(ent + 0x24));
    model_submit_culled(ent, 0);

    MEM16(ent + 0x30) = (s16)MEM32(rec + 0x1c);
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(rec + 0x10));
    mtx_rotate_zyx((u16 *)(uintptr_t)(ent + 0x2a));
    model_submit_culled(ent, 0);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

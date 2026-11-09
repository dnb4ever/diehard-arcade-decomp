/* debris_ballistic_update [0x0221c8f2]: Existing map corroborated by source: (u32 task) projectile arc; ashr_var-scaled gravity/spin/integrate by g_06096C90; land when unkC<=0x1000; deactivate on ctrl->unk12==1; draw (src symbols_band210b.txt) ; v1.09 corrected draft binding, work/decomp5/entries.asm */
#include "dh_band21.h"
/* 0x0221c8f2 - projectile/debris task handler (3-state machine + shared draw).
 * task->unk8 = entity; task->unk10 = state byte.
 *   state 0: seed sprite id unk30 = 0x46 and unk1C from the direction table
 *            0x060748c4 keyed by two bits of 0x0609856c; -> state 1 (fallthrough).
 *   state 1: per-frame integrate using the shared variable-shift ashr (0x0224a420)
 *            with shift count 0x06096c90: unk5C -= 0x240>>n; unk28 += 0x200>>n;
 *            unk24 += 0x100>>n; pos unkC += unk5C>>n; pos unk8 -= unk1C>>n; advance
 *            state once unkC <= 0x1000; (fallthrough to shared).
 *   state 2 (and fallthrough from 0/1): if the owner object (task->unk14)->unk12 == 1,
 *            consume it (clear) and deactivate the task; otherwise draw.
 *   default: draw directly.
 *   draw: push matrix stack; mtx_load_abc(); mtx_apply_vec_at8(ent);
 *         mtx_rotate_zyx_at24(ent); func_02215a2c(ent, 0); pop.
 * fidelity: HIGH - verified vs m2c_fn.py --asm; all five shift bsrf -> 0x0224a420
 *   (ashr), draw bsrf -> 0x022149aa/0x02214a40/0x02214a06/0x02215a2c. Shift inputs
 *   are explicit literals / live fields, never zero-initialised. */
extern s32  shc_shift_right_signed(s32 value, s32 count);  /* 0x0224a420 ashr variable */
extern void mtx_load_abc(void);                   /* 0x022149aa */
extern void mtx_apply_vec_at8(u32 p);             /* 0x02214a40 */
extern void mtx_rotate_zyx_at24(u32 p);           /* 0x02214a06 */
extern void model_submit_culled(u32 obj, s32 a);        /* 0x02215a2c */

void debris_ballistic_update(u32 task)
{
    u32 ent = MEM32(task + 8);
    s8 state = MEM8(task + 0x10);
    s32 n = MEM32(0x06096c90);
    u32 owner;

    switch (state) {
    case 0:
        MEM16(ent + 0x30) = 0x46;
        MEM32(ent + 0x1C) = MEM32(0x060748c4 +
            (((MEM32(0x0609856c) & 4) >> 1) + (MEM32(0x0609856c) & 1)) * 4);
        MEM8(task + 0x10)++;
        /* fallthrough */
    case 1:
        MEM32(ent + 0x5C) -= shc_shift_right_signed(0x240, n);
        MEM16(ent + 0x28) += shc_shift_right_signed(0x200, n);
        MEM16(ent + 0x24) += shc_shift_right_signed(0x100, n);
        MEM32(ent + 0xC)  += shc_shift_right_signed(MEM32(ent + 0x5C), n);
        MEM32(ent + 0x8)  -= shc_shift_right_signed(MEM32(ent + 0x1C), n);
        if (MEM32(ent + 0xC) <= 0x1000)
            MEM8(task + 0x10)++;
        /* fallthrough */
    case 2:
        owner = MEM32(task + 0x14);
        if (MEM8(owner + 0x12) == 1) {
            MEM8(owner + 0x12) = 0;
            MEM32(task + 0) = 0;
            return;
        }
        break;
    default:
        break;
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

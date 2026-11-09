/* prop_fade_in_sweep [0x0221b450]: Existing map corroborated by source: (u32 task) 4-state ramp: seed unk60/64 from unk26, step unk64/unk6C, bit0 of *0x06098580 gate, draw (load_abc/vec_at0/rotate_zyx) (src symbols_band210b.txt) */
/* 0x0221b450 (prop_fade_in_sweep) - task handler, 4-state ramp/draw machine for a
 * prop that eases in. base = task->unk8. State at task->unk10:
 *   0 (init): base->unk30 = *(s16*)(0x060745e4 + (s8)task->unk15*2); seed the two
 *      16.16 accumulators base->unk60 = base->unk64 = (u16)base->unk26; advance
 *      base->unk64 by (task->unk18 ? 0x1000 : 0x5000); base->unk6C = base->unk60;
 *      set phase = 1; fall into state 1.
 *   1: if global flag *0x06098580 bit0 set, clear it and advance phase; go draw.
 *   2: advance base->unk6C by (task->unk18 ? 0x80 : 0x400); once it passes
 *      base->unk64, clamp to base->unk64 and advance phase; go draw.
 *   3/other: draw.
 * draw: base->unk26 = base->unk6E; push matrix (g_06098878 += 0x30); mtx_load_abc;
 *   mtx_apply_vec_at0(base+8); mtx_rotate_zyx(base+0x24); submit_model_culled(base,0);
 *   pop matrix.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py; 4 bsrf resolved (braf_resolve) to
 *   mtx_load_abc (0x2149aa), mtx_apply_vec_at0 (0x214a50), mtx_rotate_zyx (0x2149e4),
 *   submit_model_culled (0x215a2c). The 0x21b474..0x21b4a6 gap is the literal pool
 *   (DATA). r6=1 (bit mask / phase constant) is set in a delay slot before dispatch
 *   so it is live on every path. */
#include "dh_band21.h"

extern void mtx_load_abc(void);              /* mtx_load_abc */
extern void mtx_apply_vec_at0(u8 *obj);      /* mtx_apply_vec_at0 */
extern void mtx_rotate_zyx(u16 *angles);     /* mtx_rotate_zyx */
extern void submit_model_culled(u32 obj, s32 flag);  /* func_02215a2c */

void prop_fade_in_sweep(u32 task)
{
    u32 base = MEMU32(task + 8);

    switch (MEM8(task + 16)) {
    case 0:
        MEM16(base + 0x30) = MEM16(0x060745e4 + (s8)MEM8(task + 15) * 2);
        MEM32(base + 0x60) = (u16)MEM16(base + 0x26);
        MEM32(base + 0x64) = (u16)MEM16(base + 0x26);
        MEM32(base + 0x64) += (MEM8(task + 18) != 0) ? 0x1000 : 0x5000;
        MEM32(base + 0x6c) = MEM32(base + 0x60);
        MEM8(task + 16) = 1;
        /* fallthrough */
    case 1:
        if (MEM32(DH_SCENE_EVENT_FLAGS_ADDR) & 1) {
            MEM32(DH_SCENE_EVENT_FLAGS_ADDR) &= ~1;
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    case 2:
        MEM32(base + 0x6c) += (MEM8(task + 18) != 0) ? 0x80 : 0x400;
        if (MEM32(base + 0x6c) > MEM32(base + 0x64)) {
            MEM32(base + 0x6c) = MEM32(base + 0x64);
            MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        }
        break;
    default:
        break;
    }

    MEM16(base + 0x26) = MEM16(base + 0x6e);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(base + 8));
    mtx_rotate_zyx(PTR(u16, base + 0x24));
    submit_model_culled(base, 0);
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

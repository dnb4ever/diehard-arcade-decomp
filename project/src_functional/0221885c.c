/* prop_tick_scrolling_pair [0x0221885c]: Two-piece draw/cull against view-task coordinate, selected by task lane. */
/* 0x0221885c (draw_scrolling_prop) - task handler for a 2-piece scenery prop that
 * scrolls/culls against a reference object. base = task->unk8;
 * ref = (*0x0609882c)->unk8->... (ref = MEM32(MEM32(0x0609882c)+8)).
 * State at task->unk10:
 *   0 (init): choose a lane from task->unk15. If non-zero: base->unk60 = 2,
 *      base->unk30 = *(s16*)(0x06054756), base->unk10 = 0x90000. Else: base->unk60 = 0,
 *      base->unk30 = *(s16*)0x06054754, base->unk8 = 0x390000. Then bump phase,
 *      base->unk20 = 0x100, and fall into the state-1 cull test.
 *   1: cull test - delta = (lane ? ref->unk10 : ref->unk8) - (lane ? base->unk10
 *      : base->unk8); if |delta| < 0x30000 return (off-screen), else draw.
 *   >=2: draw unconditionally.
 * draw: push a matrix frame (g_06098878 += 0x30), then twice
 *   {mtx_load_abc; mtx_apply_vec_at0(&tbl[idx]); submit_model_culled(base,0)} over the
 *   12-byte-stride table at 0x06054758, with idx = (s8)(u8)(b*12) and
 *   idx2 = (s8)(u8)((b+1)*12) where b = (s8)base->unk63 - note the ROM applies exts.b
 *   AFTER the *12 scaling, so the offset wraps through the low signed byte.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - reconstructed from fn.py; 6 bsrf resolved (braf_resolve) to
 *   mtx_load_abc (0x2149aa x2), mtx_apply_vec_at0 (0x214a50 x2), submit_model_culled
 *   (0x215a2c x2). Repair: the exts.b at 0x218916/0x21893c truncates the already
 *   scaled (byte*12) value, not the raw index; with b=0x25 the second piece's vector
 *   address is 0x06054758 + (s8)(0x1C8) = 0x06054720, which the previous C missed.
 * The 0x21887a..0x21889a gap is this TU's literal pool (DATA). */
#include "dh_band21.h"

extern void mtx_load_abc(void);              /* mtx_load_abc */
extern void mtx_apply_vec_at0(u8 *obj);      /* mtx_apply_vec_at0 */
extern void submit_model_culled(u32 obj, s32 flag);  /* func_02215a2c */

void prop_tick_scrolling_pair(u32 task)
{
    u32 base = MEMU32(task + 8);
    u32 ref  = MEMU32(MEMU32(DH_VIEW_TASK_ADDR) + 8);
    s32 b;
    s32 idx;

    switch (MEM8(task + 16)) {
    case 0:
        if (MEM8(task + 15) != 0) {
            MEM32(base + 0x60) = 2;
            MEM16(base + 0x30) = MEM16(0x06054756);
            MEM32(base + 0x10) = 0x90000;
        } else {
            MEM32(base + 0x60) = 0;
            MEM16(base + 0x30) = MEM16(0x06054754);
            MEM32(base + 0x08) = 0x390000;
        }
        MEM8(task + 16) = (s8)(MEM8(task + 16) + 1);
        MEM16(base + 0x20) = 0x100;
        /* fallthrough */
    case 1: {
        s32 delta;
        if (MEM8(task + 15) != 0)
            delta = MEM32(ref + 0x10) - MEM32(base + 0x10);
        else
            delta = MEM32(ref + 0x08) - MEM32(base + 0x08);
        if (delta < 0)
            delta = -delta;
        if (delta < 0x30000)
            return;
        break;
    }
    default:
        break;
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;

    mtx_load_abc();
    b = (s8)MEM8(base + 0x63);
    idx = (s8)(u8)((s32)b * 12);
    mtx_apply_vec_at0((u8 *)(uintptr_t)(0x06054758 + idx));
    submit_model_culled(base, 0);

    mtx_load_abc();
    idx = (s8)(u8)(((s32)b + 1) * 12);
    mtx_apply_vec_at0((u8 *)(uintptr_t)(0x06054758 + idx));
    submit_model_culled(base, 0);

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

/* draw_model_list [0x0221fd7a]: Existing map corroborated by source: (u32 task) walk g_06090954[(s8)task->unk15] model-record list (stride 8), push/pop matrix frames, submit each; count at rec+6 (src symbols_band210b.txt) */
/* 0x0221fd7a (draw_model_list) - task handler that renders a list of sub-model
 * records hung off an object. base = task->unk8 (the live object). Pushes a matrix
 * frame (g_06098878 is the matrix-stack depth cursor in bytes, +0x30 per level),
 * loads the base ABC matrix and applies the object vector at base+8, then walks the
 * model-record array g_06090954[(s8)task->unk15] (stride 8). For each record whose
 * s16 count at +6 is > 0: push a duplicate matrix frame, apply that record's angles,
 * copy the count into base->unk30, and submit the culled model
 * (func_02215a2c(base, 4)); then pop one frame. Pops the outer frame on exit.
 * ABI: r4 = task; no return.
 * fidelity: HIGH - m2c choked on the inline PIC pools (bra target inside the loop);
 *   reconstructed from fn.py disassembly. bsrf r2/r3 resolved (braf_resolve) to
 *   mtx_load_abc (0x2149aa) and mtx_apply_vec_at8 (0x214a40); jsr @r9/@r10/@r11 are
 *   mova+add PIC: r10->0x214a2e (mtx_stack_push_dup), r9->0x214a60
 *   (mtx_apply_angles_hi), r11->0x215a2c (submit_model_culled). mov.b at +15 is
 *   sign-extended (table index). */
#include "dh_band21.h"

extern void mtx_load_abc(void);            /* mtx_load_abc */
extern void mtx_stack_push_dup(void);      /* mtx_stack_push_dup */
extern void mtx_apply_vec_at8(u8 *obj);    /* mtx_apply_vec_at8 */
extern void mtx_apply_angles_hi(u8 *obj);  /* mtx_apply_angles_hi */
extern void model_submit_culled(u32 obj, s32 flag);

extern s32 g_matrix_stack_cursor;      /* matrix-stack depth cursor (bytes) */
extern u32 g_06090954[];    /* model-record pointer table, indexed by (s8)task->unk15 */

void draw_model_list(u32 task)
{
    u32 base = MEMU32(task + 8);
    u8 *rec;

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8((u8 *)(uintptr_t)base);

    rec = PTR(u8, g_06090954[(s8)MEM8(task + 15)]);

    while (MEM16((u32)(uintptr_t)rec + 6) > 0) {
        mtx_stack_push_dup();
        mtx_apply_angles_hi(rec);
        MEM16(base + 0x30) = MEM16((u32)(uintptr_t)rec + 6);
        model_submit_culled(base, 4);
        rec += 8;
        g_matrix_stack_cursor -= 0x30;
    }

    g_matrix_stack_cursor -= 0x30;
}

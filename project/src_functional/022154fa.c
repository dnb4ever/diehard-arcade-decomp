/* model_draw_by_matrix_mode [0x022154fa]: Selects matrix setup, culls the object and emits or queues its model draw. */
#include "dh_band21.h"
/* 0x022154fa - model/quad draw dispatcher (effect-draw cluster). Selected by
 * (arg3 & 3): class 0 pushes the matrix stack (*0x06098878 += 0x30) and loads the
 * default matrix (func_02213fb4); class 1 pushes and runs func_022149aa; both then
 * apply func_02214a40(obj). Classes 0/1/2 test visibility via func_02215630(); if
 * culled they pop (classes 0/1) and return. Otherwise func_02214a06(obj),
 * func_02213434 (matrix_copy_reverse), take the sort depth from
 * (*0x06098878)->unk2c, and (classes 0/1) pop the stack. The common tail gates on
 * 0x10000 <= depth < 0x1fbfff, projects the quad corners via func_02213a70, and
 * emits a draw record at *0x060cf0c8 (advancing it by -0x24), bumping 0x0609881c.
 * tbl = 0x060d28cc + map[obj->unk30]*0x10, map = *0x060cf0cc (s16 by obj->unk30).
 * fidelity: HIGH - all bsrf/jsr targets resolved; callee ABIs confirmed (213fb4/
 *   2149aa/213434 no-arg, 215630 no-arg returns cull flag). Class 3 (default) keeps
 *   r12 = handler addr 0x213434, which always fails the depth gate (draws nothing);
 *   modelled as such to preserve the pre-gate map reads. */

extern void load_matrix_default_raw_entry(void);          /* load_matrix_default */
extern void mtx_load_abc_entry(void);
extern void mtx_apply_vec_at8_entry(u32 obj);
extern void mtx_rotate_zyx_at24_entry(u32 obj);
extern void matrix_copy_rowswap_raw_entry(void);          /* matrix_copy_reverse */
extern s32  matrix_clip_test_raw_entry(void);          /* cull test */
extern void project_quad_corners_raw_entry(s32 w, s32 h, s32 sx, s32 sy); /* project_quad_corners */

void model_draw_by_matrix_mode(u32 obj, s32 arg1, s32 arg2, s32 arg3)
{
    s32 depth = 0x213434;   /* r12: handler addr, overwritten in classes 0/1/2 */

    switch (arg3 & 3) {
    case 0:
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
        load_matrix_default_raw_entry();
        mtx_apply_vec_at8_entry(obj);
        if (matrix_clip_test_raw_entry() != 0) {
            MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
            return;
        }
        mtx_rotate_zyx_at24_entry(obj);
        matrix_copy_rowswap_raw_entry();
        depth = MEM32(MEM32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c);
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
        break;
    case 1:
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
        mtx_load_abc_entry();
        mtx_apply_vec_at8_entry(obj);
        if (matrix_clip_test_raw_entry() != 0) {
            MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
            return;
        }
        mtx_rotate_zyx_at24_entry(obj);
        matrix_copy_rowswap_raw_entry();
        depth = MEM32(MEM32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c);
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
        break;
    case 2:
        if (matrix_clip_test_raw_entry() != 0) {
            return;
        }
        matrix_copy_rowswap_raw_entry();
        depth = MEM32(MEM32(DH_MATRIX_STACK_CURSOR_ADDR) + 0x2c);
        break;
    default:
        break;
    }

    {
        u32 tbl = 0x060d28cc +
                  ((u32)(s32)MEM16(0x060cf0cc + (MEM16(obj + 0x30) << 1)) << 4);
        u32 rec;

        if (depth < 0x10000)   return;
        if (depth >= 0x1fbfff) return;

        project_quad_corners_raw_entry((u16)MEM16(tbl + 4), (u16)MEM16(tbl + 6), arg1, arg2);

        rec = MEM32(0x060cf0c8);
        MEM32(rec + 0x20) = (arg3 & 4) ? depth : depth + 0x200000;
        MEM16(rec + 0)    = (s16)(MEMU16(obj + 0x22) | 2);
        MEM16(rec + 4)    = (s16)(MEMU16(tbl + 8) | MEMU16(obj + 0x20));
        MEM16(rec + 6)    = 0;
        MEM16(rec + 8)    = MEM16(tbl + 0);
        MEM16(rec + 10)   = MEM16(tbl + 2);
        MEM32(rec + 0xc)  = MEM32(DH_TRANSFORMED_VECTOR_ADDR + 0);
        MEM32(rec + 0x10) = MEM32(DH_TRANSFORMED_VECTOR_ADDR + 4);
        MEM32(rec + 0x14) = MEM32(DH_TRANSFORMED_VECTOR_ADDR + 8);
        MEM32(rec + 0x18) = MEM32(DH_TRANSFORMED_VECTOR_ADDR + 12);
        MEM32(0x0609881c) = MEM32(0x2609881c) + 1;
        MEM32(0x060cf0c8) = rec - 0x24;
    }
}

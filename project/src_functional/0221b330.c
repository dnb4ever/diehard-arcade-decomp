/* model_draw_composite_parts [0x0221b330]: Draws four base pieces and optional rotating appendages from parallel model/offset tables. */
/* 0x0221b330 - composite multi-part model draw (object_post_update), tail-called by the
 * keyframe mover / gravity / path tasks. entity = r4. Reads an 8-entry s16 model-id stream
 * at 0x060745A4 and a vec3 append-offset table at 0x060745B4 (stride 12).
 *   Base frame: push a matrix frame (g_06098878 += 0x30), load the cached matrix, translate
 *     by entity+8, rotate by the ZYX angles at entity+0x24; if entity->unk36 != 0 scale the
 *     3x3 by (0x1999,0x1999,0x1999). Then submit four base model parts (ids[0..3]), each
 *     written to entity->unk30 before the draw.
 *   Appendages (each: push-dup matrix, set model id, translate from the vec table, rotate,
 *     submit, pop): part C1 ids[4] at vec+0, entity->unk2C += 0x1000 then rotate_y; part C2
 *     ids[5] at vec+12, entity->unk2E += 0x1800 then rotate_x. If entity->unk37 != 0: part
 *     D1 ids[6] at vec+24 (no extra rotate), part D2 ids[7] at vec+36 (no extra rotate).
 *   Finally pop the base frame.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. braf/ptr_resolve: mtx_load_abc 0x2149aa,
 *   mtx_apply_vec_at0 0x214a50, mtx_rotate_zyx 0x2149e4, mtx_scale_cols 0x213ce4,
 *   mtx_stack_push_dup 0x214a2e, mtx_rotate_y 0x213bf4, mtx_rotate_x 0x213ba0,
 *   submit_model_culled 0x215a2c. Rotate args passed as (u16) (extu.w). */
#include "dh_band21.h"

extern void mtx_load_abc(void);                /* func_022149aa */
extern void mtx_apply_vec_at0(u8 *obj);        /* func_02214a50 */
extern void mtx_rotate_zyx(u16 *angles);       /* func_022149e4 */
extern void mtx_scale_cols(s32 x, s32 y, s32 z);  /* func_02213ce4 */
extern void mtx_stack_push_dup(void);          /* func_02214a2e */
extern void mtx_rotate_y(s32 angle);           /* func_02213bf4 */
extern void mtx_rotate_x(s32 angle);           /* func_02213ba0 */
extern void submit_model_culled(u32 obj, s32 flag);  /* func_02215a2c */

void model_draw_composite_parts(u32 entity)
{
    u32 ids = 0x060745a4;   /* s16 model-id stream cursor */
    s32 i;

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(entity + 8));
    mtx_rotate_zyx((u16 *)(uintptr_t)(entity + 0x24));
    if (MEM8(entity + 0x36) != 0)
        mtx_scale_cols(0x1999, 0x1999, 0x1999);

    for (i = 0; i < 2; i++) {
        MEM16(entity + 0x30) = MEM16(ids); ids += 2;
        submit_model_culled(entity, 0);
        MEM16(entity + 0x30) = MEM16(ids); ids += 2;
        submit_model_culled(entity, 0);
    }

    /* appendage C1 */
    mtx_stack_push_dup();
    MEM16(entity + 0x30) = MEM16(ids); ids += 2;
    mtx_apply_vec_at0((u8 *)(uintptr_t)0x060745b4);
    MEM16(entity + 0x2c) = (s16)(MEM16(entity + 0x2c) + 0x1000);
    mtx_rotate_y((u16)MEM16(entity + 0x2c));
    submit_model_culled(entity, 0);
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;

    /* appendage C2 */
    mtx_stack_push_dup();
    MEM16(entity + 0x30) = MEM16(ids); ids += 2;
    mtx_apply_vec_at0((u8 *)(uintptr_t)(0x060745b4 + 12));
    MEM16(entity + 0x2e) = (s16)(MEM16(entity + 0x2e) + 0x1800);
    mtx_rotate_x((u16)MEM16(entity + 0x2e));
    submit_model_culled(entity, 0);
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;

    if (MEM8(entity + 0x37) != 0) {
        /* appendage D1 */
        mtx_stack_push_dup();
        MEM16(entity + 0x30) = MEM16(ids); ids += 2;
        mtx_apply_vec_at0((u8 *)(uintptr_t)(0x060745b4 + 24));
        submit_model_culled(entity, 0);
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;

        /* appendage D2 (last id, no cursor advance) */
        mtx_stack_push_dup();
        MEM16(entity + 0x30) = MEM16(ids);
        mtx_apply_vec_at0((u8 *)(uintptr_t)(0x060745b4 + 36));
        submit_model_culled(entity, 0);
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

/* model_draw_title_composite [0x0221c368]: Draws three title pieces and a transformed three-part submodel. */
#include "dh_band21.h"
/* 0x0221c368 - full model-draw routine for a composite actor: three flat title
 * glyphs followed by a jointed 3-part sub-model, all through the matrix stack.
 *
 *   mtx_stack: g_06098878 += 0x30; mtx_load_abc();
 *   mtx_apply_vec_at0(ent + 8); mtx_rotate_zyx(ent + 0x24);
 *   for k=0..2: ent->unk30 = *(u16*)(0x0607460c + 2k); func_02215a2c(ent, 0);
 *   mtx_push_rotate_apply(0x06074618, {0,0,0});           (seed child transform)
 *   ent->unk40 = *(u32*)0x06098b1c + ent->unk8;           (world position write-back)
 *   ent->unk44 = *(u32*)0x06098b20 + ent->unkC;
 *   ent->unk48 = *(u32*)0x06098b24 + ent->unk10;
 *   ent->unk30 = *(u16*)0x06074612;
 *   for vec in {0x060745e8, 0x060745f4, 0x06074600}:      (3 sub-model parts)
 *       mtx_stack_push_dup(); mtx_apply_vec_at0(vec);
 *       mtx_rotate_z((u16)ent->unk2e); func_02215a2c(ent, 0); g_06098878 -= 0x30;
 *   g_06098878 -= 0x60;   (balances the initial +0x30 and the 3rd push_dup)
 *
 * fidelity: HIGH - hand-verified vs capstone disasm. Handler ptrs mova+disp:
 * 0x214a50 mtx_apply_vec_at0, 0x215a2c func_02215a2c, 0x214a2e mtx_stack_push_dup,
 * 0x213c4c mtx_rotate_z; bsrf 0x2149aa mtx_load_abc, 0x2149e4 mtx_rotate_zyx,
 * 0x21080a mtx_push_rotate_apply. Register r9/r11 are re-bound mid-body (data ptr
 * -> function ptr / advancing table ptr); resolved by hand. */
extern void mtx_load_abc(void);                      /* 0x022149aa */
extern void mtx_apply_vec_at0(u8 *obj);              /* 0x02214a50 */
extern void mtx_rotate_zyx(u16 *angles);             /* 0x022149e4 */
extern void mtx_stack_push_dup(void);                /* 0x02214a2e */
extern void mtx_rotate_z(s32 angle);                 /* 0x02213c4c */
extern void model_submit_culled(u32 obj, s32 flag);        /* 0x02215a2c */
extern void matrix_rotate_transform_vector_raw_entry(u32 vec, s16 *angles);     /* 0x0221080a mtx_push_rotate_apply */

void model_draw_title_composite(u32 ent)
{
    s16 buf[3];
    s32 k;
    u32 vec;

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)(uintptr_t)(ent + 8));
    mtx_rotate_zyx((u16 *)(uintptr_t)(ent + 0x24));

    for (k = 0; k < 3; k++) {
        MEM16(ent + 0x30) = MEM16(0x0607460c + k * 2);
        model_submit_culled(ent, 0);
    }

    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    matrix_rotate_transform_vector_raw_entry(0x06074618, buf);

    MEM32(ent + 0x40) = MEM32(DH_TRANSFORMED_VECTOR_ADDR) + MEM32(ent + 0x08);
    MEM32(ent + 0x44) = MEM32(0x06098b20) + MEM32(ent + 0x0c);
    MEM32(ent + 0x48) = MEM32(0x06098b24) + MEM32(ent + 0x10);
    MEM16(ent + 0x30) = MEM16(0x06074612);

    for (vec = 0x060745e8; vec <= 0x06074600; vec += 12) {
        mtx_stack_push_dup();
        mtx_apply_vec_at0((u8 *)(uintptr_t)vec);
        mtx_rotate_z((u16)MEM16(ent + 0x2e));
        model_submit_culled(ent, 0);
        if (vec != 0x06074600)
            MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
    }
    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x60;
}

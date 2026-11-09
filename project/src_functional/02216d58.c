/* stage_draw_visible_cells [0x02216d58]: Scans a camera-centred grid window, culls cell centres and submits visible stage cells. */
#include "dh_band21.h"
/* 0x02216d58 - visible-cell iterator over a 32x32 grid. Centre cell (a,b) comes from
 * the view record view = (*0x0609882c)->unk8: a = (s16)((view->unk8 >> 2) >> 16),
 * b = (s16)((view->unk10 >> 2) >> 16); radius R = *0x060b3d58. Scans the clamped
 * window rows [max(a-R,0) .. min(a+R,31)) x cols [max(b-R,0) .. min(b+R,31)) of the
 * grid at 0x060b3d5c (col stride 0x80, row stride 4). For each non-zero cell it
 * transforms the cell-centre (col/row in 14.18, offset +0x20000) via mtx_apply_vec
 * (func_02213538) into g_06098b1c, runs a 5-way frustum/near test on the result, and
 * on success calls func_02216ecc(cell, projected_x). Matrix stack is pushed
 * (*0x06098878 += 0x30, func_022149aa) around the scan and popped after.
 * fidelity: HIGH - bsrf targets resolved; clamps, 14.18 cell centres, the signed
 *   frustum constants, and the 2-arg func_02216ecc call (r6 is clobbered) modelled. */

extern void mtx_load_abc_entry(void);
extern void matrix_transform_to_scratch(s32 x, s32 y, s32 z);   /* mtx_apply_vec -> g_06098b1c */
extern void stage_draw_geometry_list(s32 cell, s32 x);

void stage_draw_visible_cells(void)
{
    u32 view = MEM32(MEM32(DH_VIEW_TASK_ADDR) + 8);
    s32 R = MEM32(0x060b3d58);
    s32 a = (s16)((u32)(MEM32(view + 8)    >> 2) >> 16);
    s32 b = (s16)((u32)(MEM32(view + 0x10) >> 2) >> 16);

    s32 ylo = a - R; if (ylo < 0)  ylo = 0;
    s32 yhi = a + R; if (yhi > 31) yhi = 31;
    s32 xlo = b - R; if (xlo < 0)  xlo = 0;
    s32 xhi = b + R; if (xhi > 31) xhi = 31;
    s32 x;

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc_entry();

    for (x = xlo; x < xhi; x++) {
        u32 row = 0x060b3d5c + (u32)(x * 0x80);
        s32 x_depth = (x << 18) + 0x20000;
        s32 y;
        for (y = ylo; y < yhi; y++) {
            s32 cell = MEM32(row + y * 4);
            if (cell != 0) {
                s32 X, Y, Z;
                matrix_transform_to_scratch((y << 18) + 0x20000, 0, x_depth);
                X = MEM32(DH_TRANSFORMED_VECTOR_ADDR);
                Y = MEM32(0x06098b20);
                Z = MEM32(0x06098b24);
                if ((Z + X) > (s32)0xfffc8000 &&
                    (Z - X) > (s32)0xfffc8000 &&
                    (Y + Z) > (s32)0xfffb8000 &&
                    (Z - Y) > (s32)0xfffb8000 &&
                    Z       > (s32)0xfffe8000) {
                    stage_draw_geometry_list(cell, X);
                }
            }
        }
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

/* model_submit_culled [0x02215a2c]: Frustum/depth culls model, then stages transform command or submits immediately. */
/* 0x02215a2c (submit_model_culled) - visibility-test a model and submit it for
 * drawing. The model id arg0->unk30 indexes the clip table 0x0609ebd0; a valid
 * entry (0..0x618) is required. The current transform top *0x06098878 is then
 * screen-bounds / near-plane culled: with x = m[0x2c], c = m[0x0c], r = m[0x1c],
 * all of (x+c), (x-c), (r+x), (x-r) must be >= -0x18000 and x >= 0x1999, else drop.
 * It then chooses a submission path by the transform-ring fill level
 * depth = (*0x060e0dd4 - *0x260e0dd0) / 4:
 *   depth < 3 : stage a transform record in the ring at *0x060da0c4 (fields 0x6c/
 *               0x70/0x74/0x78 = arg1, unk30, unk32, unk20), run matrix_copy_reverse_to on it,
 *               advance the ring by 0x80, append opcode 6 to the list at *0x060e0dd4,
 *               and bump counter 0x06098820.
 *   depth >= 3: run matrix_copy_reverse(), draw immediately via func_02215cb4(arg0, arg1),
 *               and bump counter 0x06098824.
 * fidelity: HIGH - clip-table range, signed bound compares (cmp/ge vs -0x18000),
 *   depth/4 via (d<0?d+3:d)>>2, in-band calls resolved (matrix_copy_reverse_to/02213474/02215cb4). */
#include "dh_band21.h"

extern void matrix_copy_reverse_to(u32 rec);
extern void matrix_copy_reverse(void);
extern void model_build_quad_list(u32 model, s32 arg1);

void model_submit_culled(u32 arg0, s32 arg1) {
    s16 v = MEM16(0x0609ebd0 + MEM16(arg0 + 0x30) * 2);
    u32 m;
    s32 x, c, depth;

    if (v < 0 || v > 0x618) return;

    m = MEM32(DH_MATRIX_STACK_CURSOR_ADDR);
    x = MEM32(m + 0x2c);
    c = MEM32(m + 0x0c);
    if (x + c < -0x18000) return;
    if (x - c < -0x18000) return;
    if (MEM32(m + 0x1c) + x < -0x18000) return;
    if (x - MEM32(m + 0x1c) < -0x18000) return;
    if (x < 0x1999) return;

    depth = MEM32(0x060e0dd4) - MEM32(0x260e0dd0);
    if (depth < 0) depth += 3;
    if ((depth >> 2) < 3) {
        u32 q = MEM32(0x060da0c4);
        MEM32(q + 0x6c) = arg1;
        MEM32(q + 0x70) = MEM16(arg0 + 0x30);
        MEM32(q + 0x74) = MEM16(arg0 + 0x32);
        MEM32(q + 0x78) = MEM16(arg0 + 0x20);
        matrix_copy_reverse_to(MEM32(0x060da0c4));
        MEM32(0x060da0c4) += 0x80;
        {
            u32 p = MEM32(0x060e0dd4);
            MEM32(0x060e0dd4) = p + 4;
            MEM32(p) = 6;
        }
        MEM32(0x06098820) += 1;
    } else {
        matrix_copy_reverse();
        model_build_quad_list(arg0, arg1);
        MEM32(0x06098824) += 1;
    }
}

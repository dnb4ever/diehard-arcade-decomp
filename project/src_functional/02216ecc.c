/* stage_draw_geometry_list [0x02216ecc]: Walks next-linked geometry nodes, loads pose into static control and submits model. */
#include "dh_band21.h"
/* 0x02216ecc - render a singly-linked list of geometry nodes.
 * Walks the list from the head, following node->unk0 (next pointer). For each node
 * it loads the node's position (unk4/unk8/unkC) and orientation (unk10/unk12/unk14
 * + unk16) into the shared static-geometry control block at 0x060b655c, then builds
 * and submits the transformed geometry:
 *   matrix_stack_push(); mtx_apply_vec_at0(ctrl+8); mtx_rotate_zyx(ctrl+0x24);
 *   func_02215a2c(ctrl, 0); and retires the matrix stack (0x06098878 -= 0x30).
 * fidelity: HIGH - verified vs m2c_fn.py --asm; the four PIC jsr targets resolve
 *   to 0x02214a2e/0x02214a50/0x022149e4/0x02215a2c via mova+offset. */
extern void matrix_stack_push(void);        /* 0x02214a2e */
extern void mtx_apply_vec_at0(u32 p);        /* 0x02214a50 */
extern void mtx_rotate_zyx(u32 p);           /* 0x022149e4 */
extern void model_submit_culled(u32 obj, s32 a);   /* 0x02215a2c */

void stage_draw_geometry_list(u32 head)
{
    u32 node = head;

    while (node != 0) {
        MEM32(0x060b655c + 0x8)  = MEM32(node + 0x4);
        MEM32(0x060b655c + 0xC)  = MEM32(node + 0x8);
        MEM32(0x060b655c + 0x10) = MEM32(node + 0xC);
        MEM16(0x060b655c + 0x24) = MEM16(node + 0x10);
        MEM16(0x060b655c + 0x26) = MEM16(node + 0x12);
        MEM16(0x060b655c + 0x28) = MEM16(node + 0x14);
        MEM16(0x060b655c + 0x30) = MEM16(node + 0x16);
        matrix_stack_push();
        mtx_apply_vec_at0(0x060b655c + 8);
        mtx_rotate_zyx(0x060b655c + 0x24);
        model_submit_culled(0x060b655c, 0);
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
        node = MEM32(node + 0x0);
    }
}

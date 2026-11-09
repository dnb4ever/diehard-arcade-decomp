/* model_draw_segment_list_task [0x0221fe00]: Task completion flag cancels; otherwise list nodes provide model ID and rotation per draw. */
/* 0x0221fe00 - segmented-model draw task: renders the entity's model once per node of a
 * global parts list, each node supplying its own rotation and model id. task (r4);
 * entity = task->unk8.
 *   If task->unk12 != 0, deactivate: task->unk0 = 0, return.
 *   Otherwise push a matrix frame (g_06098878 += 0x30), load the cached base matrix
 *   (mtx_load_abc) and apply the entity's own translation (mtx_apply_vec_at8(entity)).
 *   Walk the node list head at g_06090968 (stride 8) while node->+6 (s16) > 0: duplicate
 *   the current matrix (mtx_stack_push_dup), apply the node's high-angle rotation
 *   (mtx_apply_angles_hi(node)), set the entity model id (entity->unk30 = node->+6), submit
 *   the model (submit_model_culled(entity, 0)), then pop the duplicated frame
 *   (g_06098878 -= 0x30). Finally pop the base frame (g_06098878 -= 0x30).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. node->+6 is a signed halfword (cmp/pl); the
 *   per-node push_dup balances the per-iteration pop, and the pre-loop push balances the
 *   final pop. braf/ptr_resolve: mtx_load_abc 0x2149aa, mtx_apply_vec_at8 0x214a40,
 *   mtx_stack_push_dup 0x214a2e, mtx_apply_angles_hi 0x214a60, submit_model_culled 0x215a2c. */
#include "dh_band21.h"

extern void mtx_load_abc(void);                /* func_022149aa */
extern void mtx_apply_vec_at8_entry(void *obj);          /* mtx_apply_vec_at8 */
extern void matrix_push_duplicate_raw_entry(void);               /* mtx_stack_push_dup */
extern void matrix_transform_angle_vector_raw_entry(void *node);         /* mtx_apply_angles_hi */
extern void submit_model_culled(u32 obj, s32 flag); /* func_02215a2c */

void model_draw_segment_list_task(void *task)
{
    u32 t = (u32)(uintptr_t)task;
    u32 entity = MEMU32(t + 8);
    u32 node;

    if (MEM8(t + 0x12) != 0) {
        MEM32(t + 0) = 0;
        return;
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8_entry((void *)(uintptr_t)entity);

    node = MEMU32(0x06090968);
    while (MEM16(node + 6) > 0) {
        matrix_push_duplicate_raw_entry();
        matrix_transform_angle_vector_raw_entry((void *)(uintptr_t)node);
        MEM16(entity + 0x30) = MEM16(node + 6);
        submit_model_culled(entity, 0);
        MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
        node += 8;
    }

    MEM32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

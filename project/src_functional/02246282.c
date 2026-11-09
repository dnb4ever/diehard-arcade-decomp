/* actor_draw_transformed [0x02246282]: Pushes the cached matrix, applies actor position and angles, submits its model and pops. */
#include "dh.h"
/* 0x02246282 - func_02246282(task): draw-one-actor via the matrix stack. Pushes
 * a 48-byte (12-long) matrix frame, loads the cached matrix, applies the task's
 * position+rotation, runs the effect/draw helper, then pops the frame:
 *   g_06098878 += 48;                          // push matrix stack
 *   mtx_load_abc();                            // load cached matrix
 *   mtx_apply_vec_at0(task + 8);               // translate by task->8 vec
 *   mtx_rotate_zyx(task + 36);                 // rotate by angles at task->36
 *   func_02215a2c(task, 0);                    // effect/draw helper
 *   g_06098878 -= 48;                          // pop matrix stack
 * fidelity: HIGH - all 4 bsrf resolve. g_06098878 is the matrix-stack pointer
 * (byte granularity; one frame = 48 bytes). mtx_apply_vec_at0 receives task+8,
 * mtx_rotate_zyx receives task+36; func_02215a2c receives (task, 0). */
extern u32  g_matrix_stack_cursor;                         /* matrix-stack pointer */
extern void mtx_load_abc_entry(void);                /* 0x022149aa mtx_load_abc */
extern void matrix_transform_vector_raw_entry(void *vec);           /* 0x02214a50 mtx_apply_vec_at0 */
extern void matrix_rotate_zyx_raw_entry(void *ang);           /* 0x022149e4 mtx_rotate_zyx */
extern void model_submit_culled(void *task, s32 arg); /* 0x02215a2c effect/draw helper */

void actor_draw_transformed(void *task)
{
    g_matrix_stack_cursor += 48;
    mtx_load_abc_entry();
    matrix_transform_vector_raw_entry((u8 *)task + 8);
    matrix_rotate_zyx_raw_entry((u8 *)task + 36);
    model_submit_culled(task, 0);
    g_matrix_stack_cursor -= 48;
}

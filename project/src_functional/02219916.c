/* event_actor_draw_task [0x02219916]: Initializes model from task bytes; completion byte cancels, otherwise matrix/model draw. */
#include "dh_work.h"
#include "dh.h"
/* 0x02219916 evt_actor_render - scripted-event actor per-frame draw step.
 * arg0 = task; task->unk8 = entity; task->unk10 = state; task->unk13 = done flag.
 *   if (task->unk10 == 0) {
 *       entity->unk30 = (u16)((task->unkF << 8) + task->unk12);   (initial angle/pose)
 *       task->unk10++;
 *   }
 *   if (task->unk13 != 0) { task->unk0 = 0; return; }   (terminate when flagged)
 *   g_06098878 += 0x30;          (push draw context/priority)
 *   mtx_load_abc(task);
 *   mtx_apply_vec_at0(&entity->unk8);
 *   mtx_rotate_zyx(&entity->unk24);
 *   func_02215a2c(entity, 0);
 *   g_06098878 -= 0x30;          (pop)
 * fidelity: HIGH - bsrf targets resolved (0x2149aa task, 0x214a50 entity+8,
 * 0x2149e4 entity+36, 0x215a2c entity/0); unkF/unk12 unsigned bytes; unk30 16-bit;
 * g_06098878 draw-context accumulator. Matches m2c --asm. */
extern void mtx_load_abc(void *task);
extern void mtx_apply_vec_at0(void *p);
extern void mtx_rotate_zyx(void *p);
extern void model_submit_culled(void *entity, s32 arg);
extern s32  g_matrix_stack_cursor;

void event_actor_draw_task(void *task) {
    void *entity = TASK_PTR(task, work);

    if (TASK_S8(task, state) == 0) {
        M16(entity, 0x30) = (s16)(((u32)MU8(task, 0xF) << 8) + MU8(task, 0x12));
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    if (M8(task, 0x13) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc(task);
    mtx_apply_vec_at0((void *)((u8 *)entity + 0x8));
    mtx_rotate_zyx((void *)((u8 *)entity + 0x24));
    model_submit_culled(entity, 0);
    g_matrix_stack_cursor -= 0x30;
}

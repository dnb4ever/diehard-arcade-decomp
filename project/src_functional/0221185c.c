/* actor_tick_spin_popup [0x0221185c]: Spinning model scales up, waits, scales down and cancels task. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221185c (actor_update_spin_popup) - entity-behaviour state machine; a spinning
 * model that scales up, holds, scales down, then despawns. Installed as a task
 * handler: task->unk8 = entity, task->unk10 = state byte.
 *
 * state 0 (init): zero spin accumulators (unk24/26/28); pick a random anim frame
 *   index unk5C in 0..3 and seed unk30 from table g_0605333c; unk20 = 0x100;
 *   unk60 (Y scale) = 0; random spin rate unk2C = 0xF000 + (rng & 0x1FFF);
 *   advance task->unk10 to 1 and fall through into the update.
 * state 1 (update): unk26 += unk2C (spin); cycle unk5C through 0..3 and refresh
 *   unk30 from the table; decrement life unk58. While unk58 > 6 ease unk60 toward
 *   0x24000; while 0 < unk58 <= 6 ease unk60 toward 0; when unk58 <= 0 deactivate
 *   (task->unk0 = 0) and skip drawing.
 * any other state / surviving update: draw - push matrix stack, load base matrix,
 *   apply entity translation (unk8..) + rotation (unk24..), scale columns by
 *   (0x10000, unk60, 0x10000), submit the culled model, pop the stack.
 *
 * ABI: r4 = task. No return value.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; all bsrf targets resolved;
 * arithmetic (>>3 eases, s16 spin wrap, table stride) exact. */

extern s32  game_random_next(void);                   /* rng_next */
extern void mtx_load_abc(void);                   /* mtx_load_abc (arg ignored) */
extern void mtx_apply_vec_at8(void *entity);           /* apply matrix to vec @+8  */
extern void mtx_rotate_zyx_at24(void *entity);           /* rotate by u16 angles @+0x24 */
extern void matrix_scale_columns(s32 x, s32 y, s32 z);    /* mtx_scale_cols */
extern void model_submit_culled(void *entity, s32 flag); /* submit_model_culled */

extern s16 g_0605333c[];    /* anim-frame offset table (s16) */
extern s32 g_matrix_stack_cursor;      /* active matrix-stack pointer */

void actor_tick_spin_popup(void *task)
{
    void *ent = TASK_PTR(task, work);
    s8 state = TASK_S8(task, state);

    if (state == 0) {
        M16(ent, 0x24) = 0;
        M16(ent, 0x26) = 0;
        M16(ent, 0x28) = 0;
        M32(ent, 0x5C) = game_random_next() & 3;
        M16(ent, 0x30) = g_0605333c[M32(ent, 0x5C)];
        M16(ent, 0x20) = 0x100;
        M32(ent, 0x60) = 0;
        M16(ent, 0x2C) = (s16)((game_random_next() & 0x1FFF) + 0xF000);
        TASK_S8(task, state) = 1;
    }

    if (state == 0 || state == 1) {
        s32 life;
        M16(ent, 0x26) += M16(ent, 0x2C);
        M32(ent, 0x5C) = (M32(ent, 0x5C) + 1) & 3;
        M16(ent, 0x30) = g_0605333c[M32(ent, 0x5C)];
        M32(ent, 0x58) -= 1;
        life = M32(ent, 0x58);
        if (life > 6) {
            M32(ent, 0x60) += (0x24000 - M32(ent, 0x60)) >> 3;
        } else if (life > 0) {
            M32(ent, 0x60) += (-M32(ent, 0x60)) >> 3;
        } else {
            TASK_PTR(task, handler) = 0;
            return;
        }
    }

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    matrix_scale_columns(0x10000, M32(ent, 0x60), 0x10000);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
}

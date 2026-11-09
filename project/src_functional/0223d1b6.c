/* effect_fall_task [0x0223d1b6]: Existing map corroborated by source: state machine (fall/bounce, states 0/1/2); MED matrix/draw helpers func_022149aa/02214a40/02214a06/02215a2c around g_06098878 */
#include "dh_work.h"
#include "dh.h"
/* 0x0223d1b6 (task handler) - short falling/bouncing effect state machine.
 * entity = task->unk8; state = task->unk10.
 *  state 0: unk60 = 0x28, unk64 = 0x300, ++state.
 *  state 1: unk64 -= 0x360; unk8 += unk14; unkC += unk64; unk28 += 0x1600;
 *           unk24 += 0x1200; if unkC <= 0x500 then ++state.
 *  state 2: if --unk60 == 0 clear task & return; else draw only on even unk60.
 *  (any other state / states 0,1: fall through to draw.)
 * Draw: push the matrix stack (g_06098878 += 0x30), mtx_load_abc(), transform the
 * entity (mtx_apply_vec_at8, mtx_rotate_zyx_at24) and emit it (func_02215a2c(entity, 0)), pop.
 * fidelity: HIGH on the state machine; MED on the exact roles of the matrix/draw
 * helpers (mtx_load_abc modelled as a no-arg matrix-default load, by analogy with
 * mtx_push_rotate_apply's push/pop idiom). unk60/unk64/unk8.. long, unk24/unk28 word.
 * bsrf cart 0x2149aa/0x214a40/0x214a06/0x215a2c. */
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at8(void *entity);
extern void mtx_rotate_zyx_at24(void *entity);
extern void model_submit_culled(void *entity, s32 arg1);

void effect_fall_task(void *task) {
    void *ent = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0:
        M32(ent, 0x60) = 0x28;
        M32(ent, 0x64) = 0x300;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1:
        M32(ent, 0x64) -= 0x360;
        M32(ent, 0x8) += M32(ent, 0x14);
        M32(ent, 0xC) += M32(ent, 0x64);
        M16(ent, 0x28) = (s16)(M16(ent, 0x28) + 0x1600);
        M16(ent, 0x24) = (s16)(M16(ent, 0x24) + 0x1200);
        if (M32(ent, 0xC) <= 0x500)
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 2: {
        s32 n = M32(ent, 0x60) - 1;
        M32(ent, 0x60) = n;
        if (n == 0) {
            TASK_S32(task, handler) = 0;
            return;
        }
        if (M32(ent, 0x60) & 1)
            return;
        break;
    }
    default:
        break;
    }
    M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    M32(DH_MATRIX_STACK_CURSOR_ADDR, 0) -= 0x30;
}

/* effect_tumble_task [0x0223d2a0]: Existing map corroborated by source: 3-phase gravity/tumble debris; func_02173f4 + matrix-draw cluster (func_022149aa/02214a40/02214a06/02215a2c, g_06098878 +-0x30). MED on draw-helper args (same cluster as func_0223d1b6) */
#include "dh_work.h"
#include "dh.h"
/* 0x0223d2a0 (effect task handler) - 3-phase tumbling/gravity debris effect.
 * ent = task->unk8; state selector task->unk10.
 *   state 0 (launch): ent->unk8 += ent->unk14*2; ent->unk6C = 40;
 *     ent->unk14 = ent->unk40; ent->unk1C = ent->unk48; advance.
 *   state 1 (fall): save prev pos (unk4C/50/54 <- unk8/C/10); gravity
 *     ent->unk70 -= 0x360, ent->unk18 = ent->unk70; spin unk28 += 0x200,
 *     unk24 += 0x100; func_022173f4(ent, &g_060e0e8c[unk34*16]) (anim/colour),
 *     func_0222eea2(ent) (drawlist); on bounce flag ent->unk5D: ent->unk14 =
 *     -(unk14>>1), clear unk5D; when ent->unkC <= 0x1000 advance.
 *   state 2 (settle): ent->unk6C--; if <= 0 kill (task->unk0 = 0, return);
 *     draw only on even frames (return without drawing when unk6C is odd).
 * All drawing phases: push matrix (g_06098878 += 0x30), mtx_load_abc(),
 *   mtx_apply_vec_at8(ent), mtx_rotate_zyx_at24(ent), func_02215a2c(ent, 0), pop.
 * fidelity: HIGH logic; matrix-stack draw helpers MED (same cluster as
 * func_0223d1b6 effect_fall_task). bsrf resolved 0x2173f4/0x22eea2/0x2149aa/
 * 0x214a40/0x214a06/0x215a2c. */
extern s32 g_matrix_stack_cursor;
extern u8  g_060e0e8c[];
extern void collision_sweep_pose(void *ent, void *rec);
extern void collision_queue_actor(void *entity);
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at8(void *entity);
extern void mtx_rotate_zyx_at24(void *entity);
extern void model_submit_culled(void *entity, s32 arg1);

void effect_tumble_task(void *task) {
    void *ent = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0:
        M32(ent, 0x8) += M32(ent, 0x14) * 2;
        M32(ent, 0x6C) = 40;
        M32(ent, 0x14) = M32(ent, 0x40);
        M32(ent, 0x1C) = M32(ent, 0x48);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 1:
        M32(ent, 0x4C) = M32(ent, 0x8);
        M32(ent, 0x50) = M32(ent, 0xC);
        M32(ent, 0x54) = M32(ent, 0x10);
        M32(ent, 0x70) -= 0x360;
        M32(ent, 0x18) = M32(ent, 0x70);
        M16(ent, 0x28) = (s16)(M16(ent, 0x28) + 0x200);
        M16(ent, 0x24) = (s16)(M16(ent, 0x24) + 0x100);
        collision_sweep_pose(ent, &g_060e0e8c[M8(ent, 0x34) * 16]);
        collision_queue_actor(ent);
        if (M8(ent, 0x5D) != 0) {
            M32(ent, 0x14) = -(M32(ent, 0x14) >> 1);
            M8(ent, 0x5D) = 0;
        }
        if (M32(ent, 0xC) <= 0x1000)
            TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
        break;
    case 2:
        M32(ent, 0x6C) -= 1;
        if (M32(ent, 0x6C) <= 0) {
            TASK_S32(task, handler) = 0;
            return;
        }
        if (M32(ent, 0x6C) & 1)
            return;
        break;
    default:
        break;
    }

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(ent);
    mtx_rotate_zyx_at24(ent);
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
}

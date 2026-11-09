/* debris_arc_draw_task [0x02232526]: Existing map corroborated by source: obj=task->unk8. Gravity (unk18-=0x200), integrate; kill (src symbols_band23mid.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02232526 - falling/arc debris draw task (gravity on the unk18 velocity).
 * obj = task->unk8. Snapshots the previous position into unk4C/unk50/unk54, applies
 * gravity (unk18 -= 0x200), then integrates (unk8 += unk14; unkC += unk18;
 * unk10 += unk1C). When unkC (Y) goes negative the task is killed. Otherwise, if
 * obj->unk3C bit0 is set it is prepended to the draw list, two look-at angles are
 * derived from the velocity vector into unk24/unk26, the matrix stack is pushed
 * (g_06098878 += 0x30), the cached ABC matrix loaded, the obj+8 vector applied and
 * the obj+0x24 angles rotated in. If the resulting top matrix depth (unk2C) exceeds
 * 0x60000 the model is column-scaled (0x10000,0x10000,unk1C<<3) and submitted with
 * culling. Finally the matrix stack is popped (g_06098878 -= 0x30).
 * fidelity: HIGH - all 7 bsrf targets confirmed via braf_resolve. */
extern s32  g_matrix_stack_cursor;                          /* active matrix-stack pointer */
extern void collision_queue_actor(void *entity);         /* drawlist_prepend */
extern void camera_angles_from_delta(s32 dx, s32 dy, s32 dz, void *out_a, void *out_b);
extern void mtx_load_abc(void);                 /* mtx_load_abc */
extern void mtx_apply_vec_at0(void *vec);            /* mtx_apply_vec_at0 */
extern void mtx_rotate_zyx(void *angles);         /* mtx_rotate_zyx */
extern void matrix_scale_columns(s32 x, s32 y, s32 z);  /* mtx_scale_cols */
extern void model_submit_culled(void *obj, s32 flag);  /* submit_model_culled */

void debris_arc_draw_task(void *task) {
    void *obj = TASK_PTR(task, work);
    M32(obj, 0x4C) = M32(obj, 8);
    M32(obj, 0x50) = M32(obj, 0xC);
    M32(obj, 0x54) = M32(obj, 0x10);
    M32(obj, 0x18) -= 0x200;
    M32(obj, 8)    += M32(obj, 0x14);
    M32(obj, 0xC)  += M32(obj, 0x18);
    M32(obj, 0x10) += M32(obj, 0x1C);
    if (M32(obj, 0xC) < 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    if (M32(obj, 0x3C) & 1)
        collision_queue_actor(obj);
    camera_angles_from_delta(M32(obj, 0x14), M32(obj, 0x18), M32(obj, 0x1C),
                  (u8 *)obj + 0x24, (u8 *)obj + 0x26);
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)obj + 8);
    mtx_rotate_zyx((u8 *)obj + 0x24);
    if (M32((void *)(uintptr_t)g_matrix_stack_cursor, 0x2C) > 0x60000) {
        matrix_scale_columns(0x10000, 0x10000, M32(obj, 0x1C) << 3);
        model_submit_culled(obj, 0);
    }
    g_matrix_stack_cursor -= 0x30;
}

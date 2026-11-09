/* actor_aim_transform_draw [0x022399bc]: Existing map corroborated by source: per-frame aim/skeletal (src symbols_band2399bc.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x022399bc  actor_aim_transform_draw (func_022399bc)
 *
 * Per-frame aim/skeletal transform + model submit for one actor.
 *   actor = arg; obj = actor->unk8; idx = (s8)obj->unk34;
 *   e = (*g_060995b0[idx])->unk8 (RAM record pointer, then +8 deref).
 *   rot_base = 0x06096328 + (s16)(idx*108)  (per-command Euler-angle records).
 *
 * dispatch on (s8)actor->unk10:
 *   0: full aim setup - push matrix, run the 1st command loop (rotate x/y/z by
 *      negated per-command angles), compose aim via mtx_apply_vec / lookat /
 *      angle_from_vec2 into obj->unk24..28, pop, bump actor->unk10; then tail.
 *   1: countdown obj->unk32; on <=0 reload 30 and bump actor->unk10; then tail.
 *   2: countdown obj->unk32; on <=0 clear *actor and return; else parity gate
 *      (odd -> return); even -> tail.
 *   default: tail.
 * shared tail: push matrix; mtx_load_abc; mtx_apply_vec_at0(e+8);
 *   mtx_rotate_zyx(e+0x24); vec3_array_matmul(0, g_06096c58[idx], 0); the 2nd
 *   command loop (two cursors, apply lagging rotate by one byte);
 *   mtx_rotate_zyx_at24(obj); submit_model_culled(obj,0); pop.
 *
 * NB: the jsr @r9 in the shared tail targets 0x022149e4 = mtx_rotate_zyx
 * (mtx_rotate_zyx), NOT 0x022179e4. The prior deferral was a transcription
 * error; 0x2149e4 is a standard-ABI 1-pointer-arg helper. All 20 bsrf + the
 * mova+add jsr resolved via braf_resolve.py / hand arithmetic.
 * fidelity: HIGH.
 */
extern void *g_actor_slot_tasks_legacy[]; /* RAM record-pointer table, idx (s8)obj->unk34 */
extern s32   g_06096c58[]; /* per-actor matmul scalar table, idx unk34 */
extern s32   g_transformed_vector[]; /* mtx_apply_vec output vector [b1c/b20/b24] */

extern void load_matrix_default(void);                /* load_matrix_default */
extern void mtx_load_abc(void);                /* mtx_load_abc */
extern void mtx_rotate_zyx_at24(void *obj);           /* mtx_rotate_zyx_at24 */
extern void mtx_rotate_zyx(void *angles);        /* mtx_rotate_zyx (u16[3]) */
extern void mtx_apply_vec_at0(void *vec);           /* mtx_apply_vec_at0 (s32[3]) */
extern void mtx_rotate_x(s32 angle);           /* mtx_rotate_x */
extern void mtx_rotate_y(s32 angle);           /* mtx_rotate_y */
extern void mtx_rotate_z(s32 angle);           /* mtx_rotate_z */
extern void matrix_transform_to_scratch(s32 x, s32 y, s32 z); /* mtx_apply_vec -> g_06098b1c */
extern void matrix_translate_current(s32 x, s32 y, s32 z); /* vec3_array_matmul */
extern s32  atan2_fixed(s32 a, s32 b);        /* angle_from_vec2 */
extern void camera_compute_xy_lookat(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f,
                          void *out1, void *out2); /* lookat_angles_xy */
extern void model_submit_culled(void *obj, s32 flag); /* submit_model_culled */

#define MTX_TOP (*(volatile s32 *)(uintptr_t)DH_MATRIX_STACK_CURSOR_ADDR)

void actor_aim_transform_draw(void *actor) {
    void *obj = MP(actor, 0x8);
    s32   idx = (s8)M8(obj, 0x34);
    void *e   = TASK_PTR(g_actor_slot_tasks_legacy[idx], work);
    u8   *rot_base = (u8 *)(uintptr_t)(0x06096328u + (s16)(idx * 108));

    switch (M8(actor, 0x10)) {
    case 0: {
        u8 *cmd;
        s32 vx, vy, vz;

        M16(obj, 0x32) = 70;
        MTX_TOP += 0x30;
        load_matrix_default();
        mtx_rotate_zyx_at24(obj);

        /* 1st command loop: rotate x/y/z by negated per-command angles */
        cmd = (u8 *)(uintptr_t)(0x0608c622u + MU32(obj, 0x38) * 8u);
        while (*(s8 *)cmd >= 0) {
            u8 *rec = rot_base + (s8)*cmd * 6;
            cmd++;
            mtx_rotate_x(-(s32)MU16(rec, 0x0));
            mtx_rotate_y(-(s32)MU16(rec, 0x2));
            mtx_rotate_z(-(s32)MU16(rec, 0x4));
        }

        mtx_rotate_x(-(s32)MU16(e, 0x24));
        mtx_rotate_y(-(s32)MU16(e, 0x26));
        mtx_rotate_z(-(s32)MU16(e, 0x28));

        matrix_transform_to_scratch(0x80000, 0, 0);
        camera_compute_xy_lookat(0, 0, 0,
                      g_transformed_vector[0], g_transformed_vector[1], g_transformed_vector[2],
                      (u8 *)obj + 0x28, (u8 *)obj + 0x26);
        matrix_transform_to_scratch(0, 0, 0x80000);

        vx = g_transformed_vector[0];
        vy = g_transformed_vector[1];
        vz = g_transformed_vector[2];
        load_matrix_default();
        mtx_rotate_y(-(s32)MU16(obj, 0x26));
        mtx_rotate_z(-(s32)MU16(obj, 0x28));
        matrix_transform_to_scratch(vx, vy, vz);
        M16(obj, 0x24) = (s16)atan2_fixed(g_transformed_vector[2], g_transformed_vector[1]);

        MTX_TOP -= 0x30;
        M8(actor, 0x10) = (s8)(M8(actor, 0x10) + 1);
        break;
    }
    case 1: {
        s16 cd = (s16)(M16(obj, 0x32) - 1);
        M16(obj, 0x32) = cd;
        if (cd <= 0) {
            M16(obj, 0x32) = 30;
            M8(actor, 0x10) = (s8)(M8(actor, 0x10) + 1);
        }
        break;
    }
    case 2: {
        s16 cd = (s16)(M16(obj, 0x32) - 1);
        M16(obj, 0x32) = cd;
        if (cd <= 0) {
            M32(actor, 0x0) = 0;
            return;
        }
        if (M16(obj, 0x32) & 1)
            return;
        break;
    }
    default:
        break;
    }

    /* shared draw tail */
    MTX_TOP += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)e + 0x8);
    mtx_rotate_zyx((u8 *)e + 0x24);
    matrix_translate_current(0, g_06096c58[idx], 0);

    /* 2nd command loop: primary cursor drives mtx_rotate_zyx; a lagging cursor
     * one byte behind drives mtx_apply_vec_at0. The first primary byte's apply
     * is skipped (loop entered at the rotate); the last byte's apply is never
     * reached (loop exits on the trailing negative sentinel). */
    {
        u8 *pc  = (u8 *)(uintptr_t)(0x0608c5d2u + MU32(obj, 0x38) * 8u);
        u8 *lag = pc;
        s8  c   = *(s8 *)pc++;
        for (;;) {
            mtx_rotate_zyx(rot_base + c * 6);
            if (*(s8 *)pc < 0)
                break;
            c = *(s8 *)pc++;
            {
                s8 b = *(s8 *)lag++;
                mtx_apply_vec_at0((u8 *)(uintptr_t)(0x0607976Cu
                              + (s16)M16(e, 0x5E) * 180 + b * 12));
            }
        }
    }

    mtx_rotate_zyx_at24(obj);
    model_submit_culled(obj, 0);
    MTX_TOP -= 0x30;
}

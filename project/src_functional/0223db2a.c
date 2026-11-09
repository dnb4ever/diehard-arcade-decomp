/* effect_tick_scaled_tumbling [0x0223db2a]: Task states integrate gravity/spin using global shift scale and slot-table handshake. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223db2a - projectile / tumbling-debris task state machine (task = arg0).
 *   entity = task->unk8;  state = task->unk10;  g_060e0e88 = table base.
 *   g_06096c90 = velocity-shift count (all per-frame deltas are >> this count,
 *   via func_0224a420/ashr_var; m2c renders these calls as plain adds).
 *
 * state 0 (init): entity->unk30 = g_060e1080[ g_060e0f0c[entity->unk34] ];
 *   task->unk10 = 4; fall through.
 * state 4 (arm): if g_060e0f2c[entity->unk34] != 3 return; else task->unk10 = 2.
 * state 2 (fly): gravity/spin integrate (cnt = g_06096c90):
 *   unkC8 -= 0x360>>cnt;  unk28 += 0x400>>cnt;  unk24 += 0x100>>cnt;
 *   unkC += unkC8>>cnt;   unk8  += unk78>>cnt;  unk10 += unk80>>cnt.
 *   If unkC <= 0x1000 (hit floor): if unkC8 > -0x600 settle
 *     (unk24 = rng(), unk26 = 0, unk28 = 0x4000, unkC = 0x500, state 5)
 *   else bounce (unkC = 0x1000; unkC8 = -unkC8>>1; unk78>>=1; unk80>>=1).
 *   Then fall into the state-5 check.
 * state 5 (settle): if g_060e0f2c[entity->unk34] == 5 -> task->unk10 = 6.
 * state 6 (done): func_0223cd0c(entity); task->unk0 = 0 (kill); return.
 * default: just draw.
 *
 * Draw tail (states 2, 5 and default): reserve a 0x30-byte scratch block at
 * g_06098878, mtx_load_abc(), mtx_apply_vec_at8(entity),
 * mtx_rotate_zyx_at24(entity), func_02215a2c(entity, 0), release the scratch.
 *
 * fidelity: HIGH - hand-verified vs m2c_fn --asm + braf_resolve (fn.py 0x23db2a).
 * The six state-2 bsrf are func_0224a420 (ashr_var, r0=value r1=count) - m2c drops
 * the shift; func_02210020 is the RNG (return kept in unk24). Tables are
 * g_060e0e88-relative (0f0c = +0x84, 0f2c = +0xa4). unk24/26/28 halfwords, the
 * position/velocity fields unk8/C/10/78/80/C8 longs. */
#define G16(a)  (*(s16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern s32  shc_shift_right_signed(s32 value, s32 count);   /* ashr_var */
extern s32  game_random_next(void);                    /* rng */
extern void effect_pool_release(void *entity);
extern void mtx_load_abc(void);                   /* mtx_load_abc */
extern void mtx_apply_vec_at8(void *obj);              /* mtx_apply_vec_at8 */
extern void mtx_rotate_zyx_at24(void *obj);              /* mtx_rotate_zyx_at24 */
extern void model_submit_culled(u32 obj, s32 arg1);

void effect_tick_scaled_tumbling(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 cnt;

    switch (TASK_U8(task, state)) {
    case 0:
        M16(entity, 0x30) =
            G16(0x060e1080 + G32(0x060e0f0c + M8(entity, 0x34) * 4) * 2);
        TASK_U8(task, state) = 4;
        /* fallthrough */
    case 4:
        if (G32(0x060e0f2c + M8(entity, 0x34) * 4) != 3)
            return;
        TASK_U8(task, state) = 2;
        return;
    case 2:
        cnt = G32(0x06096c90);
        M32(entity, 0xC8) -= shc_shift_right_signed(0x360, cnt);
        M16(entity, 0x28) = (s16)(M16(entity, 0x28) + shc_shift_right_signed(0x400, cnt));
        M16(entity, 0x24) = (s16)(M16(entity, 0x24) + shc_shift_right_signed(0x100, cnt));
        M32(entity, 0x0C) += shc_shift_right_signed(M32(entity, 0xC8), cnt);
        M32(entity, 0x08) += shc_shift_right_signed(M32(entity, 0x78), cnt);
        M32(entity, 0x10) += shc_shift_right_signed(M32(entity, 0x80), cnt);
        if (M32(entity, 0x0C) <= 0x1000) {
            if (M32(entity, 0xC8) > -0x600) {
                M16(entity, 0x24) = (s16)game_random_next();
                M16(entity, 0x26) = 0;
                M16(entity, 0x28) = 0x4000;
                M32(entity, 0x0C) = 0x500;
                TASK_U8(task, state) = 5;
            } else {
                M32(entity, 0x0C) = 0x1000;
                M32(entity, 0xC8) = (-M32(entity, 0xC8)) >> 1;
                M32(entity, 0x78) = M32(entity, 0x78) >> 1;
                M32(entity, 0x80) = M32(entity, 0x80) >> 1;
            }
        }
        /* fallthrough */
    case 5:
        if (G32(0x060e0f2c + M8(entity, 0x34) * 4) == 5)
            TASK_U8(task, state) = 6;
        break;
    case 6:
        effect_pool_release(entity);
        TASK_PTR(task, handler) = 0;
        return;
    default:
        break;
    }

    G32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at8(entity);
    mtx_rotate_zyx_at24(entity);
    model_submit_culled((u32)(uintptr_t)entity, 0);
    G32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;
}

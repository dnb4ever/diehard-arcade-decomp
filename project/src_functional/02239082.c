/* spin_projectile_handler [0x02239082]: Existing map corroborated by source: spinning projectile + draw. ent=task[8]; rec=g_06097d58[src->unk34*0x100]. s0: unk26/unk60 by facing; unk40=task->unk13. frame: unk40--; kill if was 0 or task->unk12; spin unk44+=0x1000 unk48+=0x1400; pos; unk30=(prev unk54 &3)+0x4F. draw: push; func_022149aa(); func_02214a50(&ent[8]); func_022149e4(&ent[0x24]); func_02213ce4(0x10000,0x20000+(sin(unk48)>>2),0x28000+((-cos(unk44))>>2)); func_02215a2c(ent,0); pop. sin=func_02213190 cos=func_022131a8. 6 bsrf resolved. */
#include "dh_work.h"
#include "dh.h"
/* 0x02239082 (callC) - spinning projectile task handler + draw.
 * ent = task->unk8; src = task->unk14->unk8; rec = &g_06097d58[src->unk34*0x100].
 *   state 0 (task->unk10 == 0): by facing (task->unkF) set ent->unk26 and the
 *     vertical bias ent->unk60: facing -> unk26 = 0x4000, unk60 = -0x3000;
 *     else unk26 = 0xC000, unk60 = 0x3000. unk40 = task->unk13 (life); advance.
 *   every frame: unk40--; if it was 0 or task->unk12 set, kill (task->unk0 = 0).
 *     Else spin unk44 += 0x1000, unk48 += 0x1400; position
 *     unk8 = rec->unk50 + unk60; unkC = rec->unk54 + 0x2000; unk10 = rec->unk58;
 *     unk54++; unk30 = (s16)((prev unk54 & 3) + 0x4F).
 *   draw: push matrix (g_06098878 += 0x30); mtx_load_abc(); translate
 *     mtx_apply_vec_at0(&ent->unk8); rotate mtx_rotate_zyx(&ent->unk24); scale
 *     func_02213ce4(0x10000, 0x20000 + (sin(unk48) >> 2),
 *     0x28000 + ((-cos(unk44)) >> 2)); render func_02215a2c(ent, 0); pop matrix.
 *     sin = sin_lookup, cos = cos_lookup.
 * fidelity: HIGH. all bsrf targets resolved. */
extern u8   g_06097d58[];
extern s32  g_matrix_stack_cursor;
extern void mtx_load_abc(void);
extern void mtx_apply_vec_at0(void *vec);
extern void mtx_rotate_zyx(void *angles);
extern s32  cos_lookup(s32 angle);
extern s32  sin_lookup(s32 angle);
extern void matrix_scale_columns(s32 x, s32 y, s32 z);
extern void model_submit_culled(void *actor, s32 arg);

void spin_projectile_handler(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 0x8);
    void *rec = &g_06097d58[M8(src, 0x34) << 8];
    s32 t, prev;
    s32 c, s;

    if (TASK_S8(task, state) == 0) {
        if (M8(task, 0xF) != 0) {
            M16(ent, 0x26) = 0x4000;
            M32(ent, 0x60) = -0x3000;
        } else {
            M16(ent, 0x26) = (s16)0xC000;
            M32(ent, 0x60) = 0x3000;
        }
        M32(ent, 0x40) = M8(task, 0x13);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }

    t = M32(ent, 0x40) - 1;
    M32(ent, 0x40) = t;
    if (t == -1 || M8(task, 0x12) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(ent, 0x44) += 0x1000;
    M32(ent, 0x48) += 0x1400;
    M32(ent, 0x8) = M32(rec, 0x50) + M32(ent, 0x60);
    M32(ent, 0xC) = M32(rec, 0x54) + 0x2000;
    M32(ent, 0x10) = M32(rec, 0x58);
    prev = M32(ent, 0x54);
    M32(ent, 0x54) = prev + 1;
    M16(ent, 0x30) = (s16)((prev & 3) + 0x4F);

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0((u8 *)ent + 0x8);
    mtx_rotate_zyx((u8 *)ent + 0x24);
    c = cos_lookup(M32(ent, 0x44));
    s = sin_lookup(M32(ent, 0x48));
    matrix_scale_columns(0x10000, 0x20000 + (s >> 2), 0x28000 + ((-c) >> 2));
    model_submit_culled(ent, 0);
    g_matrix_stack_cursor -= 0x30;
}

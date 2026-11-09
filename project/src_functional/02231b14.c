/* launched_actor_task [0x02231b14]: Existing map corroborated by source: 2-state. state0: pos from src (g_06097D58), (src symbols_band231.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x02231b14 (task handler) - launched-actor init/fly task (2 states).
 * ent = task->unk8; oent = task->unk14->unk8 (owner entity). state = (s8)task->unk10.
 * state 0 (spawn/transform/aim):
 *   src = &g_06097D58[(oent->unk34 << 8) + (u8)ent->unk5F * 0x10];
 *   ent->unk8/C/10 = src[0/4/8] (position); func_02238dde() (post-clone init);
 *   optional sound_queue_push(0xA01303FF) when g_060E0DDC;
 *   mtx_push_rotate_apply(g_06087F24, &oent->unk24) -> world vec g_06098B1C;
 *   copy g_06098B1C[0..2] to local v; ent->unk30(w)=0x17A;
 *   push matrix (g_06098878 += 0x30); mtx_load_abc();
 *   mtx_apply_vec_at0(src); mtx_apply_vec_at0(v); func_022154fc(ent,0x40000,0x20000,2);
 *   pop matrix (g_06098878 -= 0x30); func_02231a80(ent, oent) (set aim angles);
 *   ent->unk30(w)=0x179; task->unk10++.
 * state 1 (fly): t = ent->unk58(w); ent->unk58 = t-1; if (t <= 0) kill (task->unk0=0).
 *   else integrate: ent->unk8 += ent->unk14; ent->unkC += ent->unk18; ent->unk10 += ent->unk1C;
 *   func_022154fc(ent, 0x20000, 0x10000, 1).
 * default: no-op.
 * fidelity: HIGH - oent->unk34 byte<<8, ent->unk5F u8*0x10, counter unk58 word,
 *   anim unk30 word, matrix push/pop +/-0x30; all 8 bsrf resolved + direct bsr 0x231a80.
 *   mtx_apply_vec_at0(src) transforms the shared anim record in place (as in ROM). */
extern void display_mark_channel_bits(void);                            /* post-clone init */
extern void sound_queue_push(s32 cmd);                         /* sound_queue_push */
extern void mtx_push_rotate_apply(s32 *vec, u16 *angles);           /* mtx_push_rotate_apply */
extern void mtx_load_abc(void);                            /* mtx_load_abc */
extern void mtx_apply_vec_at0(void *obj);                       /* mtx_apply_vec_at0 */
extern void sprite_draw_billboard(void *entity, s32 a, s32 b, s32 c);
extern void effect_orient_from_parent(void *ent, void *params);         /* orient entity (aim angles) */
extern u8   g_06097d58[];
extern s32  g_06087f24[];   /* fixed source vector (3 longs) */
extern s32  g_transformed_vector[];   /* matrix-apply output vector */
extern s32  g_matrix_stack_cursor;     /* active-matrix stack pointer */
extern s32  g_sound_effects_enabled;

void launched_actor_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *oent = MP(TASK_PTR(task, parent), 8);
    s8 state = TASK_S8(task, state);

    switch (state) {
    case 0: {
        u8 *src = &g_06097d58[(M8(oent, 0x34) << 8) + MU8(ent, 0x5F) * 0x10];
        s32 v[3];
        M32(ent, 0x8)  = M32(src, 0);
        M32(ent, 0xC)  = M32(src, 4);
        M32(ent, 0x10) = M32(src, 8);
        display_mark_channel_bits();
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA01303FF);
        mtx_push_rotate_apply(g_06087f24, (u16 *)((u8 *)oent + 0x24));
        v[0] = g_transformed_vector[0];
        v[1] = g_transformed_vector[1];
        v[2] = g_transformed_vector[2];
        M16(ent, 0x30) = 0x17A;
        g_matrix_stack_cursor += 0x30;
        mtx_load_abc();
        mtx_apply_vec_at0(src);
        mtx_apply_vec_at0(v);
        sprite_draw_billboard(ent, 0x40000, 0x20000, 2);
        g_matrix_stack_cursor -= 0x30;
        effect_orient_from_parent(ent, oent);
        M16(ent, 0x30) = 0x179;
        TASK_S8(task, state) = (s8)(state + 1);
        return;
    }
    case 1: {
        s16 t = M16(ent, 0x58);
        M16(ent, 0x58) = (s16)(t - 1);
        if (t <= 0) {
            TASK_S32(task, handler) = 0;
            return;
        }
        M32(ent, 0x8)  += M32(ent, 0x14);
        M32(ent, 0xC)  += M32(ent, 0x18);
        M32(ent, 0x10) += M32(ent, 0x1C);
        sprite_draw_billboard(ent, 0x20000, 0x10000, 1);
        return;
    }
    default:
        return;
    }
}

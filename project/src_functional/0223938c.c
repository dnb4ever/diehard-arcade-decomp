/* projectile_toss_handler [0x0223938c]: Existing map corroborated by source: arc projectile. ent=task[8]; rec=g_06097d58[src->unk34*0x100]. s0: velocity by facing task->unkF (unk50/unk54 +-0xA00/0x400); unk40=task->unk13 (life); unk30=0x197. frame: unk40--; kill if was 0 or task->unk12; pos=rec+vel; spin unk44+=0x1C00 unk48+=0x2800; draw func_022154fc(ent,0x40000+(sin(unk44)<<1),0x40000+(sin(unk48)>>1),1). sin=func_02213190. bsrf/braf resolved. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223938c (callC) - projectile arc task handler (grenade-style toss).
 * ent = task->unk8; src = task->unk14->unk8; rec = &g_06097d58[src->unk34*0x100].
 *   state 0 (task->unk10 == 0): horizontal velocity by facing (task->unkF):
 *     facing set -> unk50 = -0xA00, unk54 = 0x400; else unk50 = 0xA00,
 *     unk54 = -0x400. unk40 = task->unk13 (life); unk30 = 0x197; advance.
 *   every frame: unk40--; if it was 0 or task->unk12 set, kill (task->unk0 = 0).
 *     Else position: unk8 = rec->unk30 + unk50; unkC = rec->unk34 + 0x3000;
 *     unk10 = rec->unk38 + unk54; spin unk44 += 0x1C00; unk48 += 0x2800.
 *   draw: func_022154fc(ent, 0x40000 + (sin(unk44)<<1), 0x40000 + (sin(unk48)>>1), 1)
 *     where sin = sin_lookup (fixed-point trig).
 * fidelity: HIGH. bsrf/braf resolved to sin_lookup (x2) and func_022154fc. */
extern u8   g_06097d58[];
extern s32  sin_lookup(s32 angle);
extern void sprite_draw_billboard(void *work, s32 a, s32 b, s32 c);

void projectile_toss_handler(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 0x8);
    void *rec = &g_06097d58[M8(src, 0x34) << 8];
    s32 t;
    s32 s48, s44;

    if (TASK_S8(task, state) == 0) {
        if (M8(task, 0xF) != 0) {
            M32(ent, 0x50) = -0xA00;
            M32(ent, 0x54) = 0x400;
        } else {
            M32(ent, 0x50) = 0xA00;
            M32(ent, 0x54) = -0x400;
        }
        M32(ent, 0x40) = M8(task, 0x13);
        M16(ent, 0x30) = 0x197;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }

    t = M32(ent, 0x40) - 1;
    M32(ent, 0x40) = t;
    if (t == -1 || M8(task, 0x12) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(ent, 0x8) = M32(rec, 0x30) + M32(ent, 0x50);
    M32(ent, 0xC) = M32(rec, 0x34) + 0x3000;
    M32(ent, 0x10) = M32(rec, 0x38) + M32(ent, 0x54);
    M32(ent, 0x44) += 0x1C00;
    M32(ent, 0x48) += 0x2800;

    s48 = sin_lookup(M32(ent, 0x48));
    s44 = sin_lookup(M32(ent, 0x44));
    sprite_draw_billboard(ent, 0x40000 + (s44 << 1), 0x40000 + (s48 >> 1), 1);
}

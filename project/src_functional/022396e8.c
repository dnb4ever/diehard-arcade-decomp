/* projectile_tick_directional_arc [0x022396e8]: Owner pose plus facing kick seeds projectile; arc integration and draw follow. */
#include "dh_work.h"
#include "dh.h"
/* 0x022396e8 (actor FSM) - directional projectile launch + arc animate. task node at r4, entity
 * at task+0x8. state 0 (init): load a pose vector (+0x8/+0xC/+0x10) from the 0x100-byte record
 * g_06097DA8[src type +0x34] (src = task+0x14 ->unk8), nudge X by +/-0x4000 depending on the
 * source facing (+0x26 == 0xC000), raise Y (+0xC += 0x1800), seed +0x40 = 0x38000, +0x44 = 0x600,
 * +0x48 = 0, advance state, fall through. state 1 (per frame): +0x40 += 0x4000, +0xC += +0x44,
 * pose +0x30 = (+0x48 >> 1) + 0x17C, and once the 11-frame counter +0x48 passes 10 mark the task
 * dead (+0x0 = 0); either way tail-branch (braf) to func_022153f6(entity, +0x40, 1). Other states
 * return.
 * fidelity: HIGH - pos/vel/+0x40/+0x44/+0x48 32-bit, +0x30 16-bit, type +0x34 8-bit, facing +0x26
 * u16; braf target resolved via braf_resolve.py (delta 0xfffdbc38 -> 0x02153f6). */
extern s8   g_06097DA8[];    /* 0x100-byte pose records indexed by entity type */
extern void sprite_draw_child(void *entity, s32 a, s32 b);
void projectile_tick_directional_arc(void *task) {
    void *entity = TASK_PTR(task, work);
    switch (TASK_S8(task, state)) {
    case 0: {
        void *src = MP(TASK_PTR(task, parent), 0x8);
        s8 *rec = &g_06097DA8[M8(src, 0x34) << 8];
        M32(entity, 0x8)  = M32(rec, 0x0);
        M32(entity, 0xC)  = M32(rec, 0x4);
        M32(entity, 0x10) = M32(rec, 0x8);
        M32(entity, 0x8) += (MU16(src, 0x26) == 0xC000) ? 0x4000 : -0x4000;
        M32(entity, 0xC) += 0x1800;
        M32(entity, 0x40) = 0x38000;
        M32(entity, 0x44) = 0x600;
        M32(entity, 0x48) = 0;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    /* fallthrough */
    case 1:
        M32(entity, 0x40) += 0x4000;
        M32(entity, 0xC)  += M32(entity, 0x44);
        M16(entity, 0x30) = (s16)((M32(entity, 0x48) >> 1) + 0x17C);
        M32(entity, 0x48) += 1;
        if (M32(entity, 0x48) > 0xA)
            TASK_S32(task, handler) = 0;
        sprite_draw_child(entity, M32(entity, 0x40), 1);  /* braf tail-branch */
        break;
    default:
        break;
    }
}

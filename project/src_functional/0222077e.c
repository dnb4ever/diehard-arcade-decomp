/* effect_tick_ballistic_sprite_source_alias [0x0222077e]: Duplicate draft bound to 0222078c; sixty-frame lifetime, gravity and spinning sprite. */
#include "dh_work.h"
#include "dh.h"
/* 0x0222077e (actor FSM) - projectile/launch task handler on the task node (state @task+0x10),
 * driving the entity at task+0x8. state 0 (one-shot init): copy position (+0x8/+0xC/+0x10) from
 * the spawn source task+0x14 -> unk8, seed a palette/attr field (+0x28 = (g_060987BC & 0xFF)<<8),
 * set +0x30 = 0xA, advance state. Every frame: bump life counter +0x32; if it passes 0x3C or the
 * Y coord (+0xC) goes negative, clear the task (+0x0) and return. Otherwise integrate gravity
 * (+0xC += +0x18; +0x18 -= 0x100) and spin two angles (+0x24 += 0x400, +0x26 += 0x300), then
 * tail-branch (braf) to func_022153f6(entity, 0x8000, 1).
 * fidelity: HIGH - widths from the loads (pos/vel 32-bit, counter/angles 16-bit, state 8-bit);
 * braf target resolved via tools/decomp/braf_resolve.py (delta 0xffff4be4 -> 0x02153f6). */
extern s32  g_frame_tick;       /* frame/seed byte source */
extern void sprite_draw_child(void *entity, s32 arg1, s32 arg2);

void effect_tick_ballistic_sprite_source_alias(void *task) {
    void *entity = TASK_PTR(task, work);
    s16 life;
    if (TASK_S8(task, state) == 0) {
        void *src = MP(TASK_PTR(task, parent), 0x8);
        M32(entity, 0x8)  = M32(src, 0x8);
        M32(entity, 0xC)  = M32(src, 0xC);
        M32(entity, 0x10) = M32(src, 0x10);
        M16(entity, 0x28) = (s16)((g_frame_tick & 0xFF) << 8);
        M16(entity, 0x30) = 0xA;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    life = (s16)(M16(entity, 0x32) + 1);
    M16(entity, 0x32) = life;
    if (life > 0x3C || M32(entity, 0xC) < 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(entity, 0xC)  += M32(entity, 0x18);
    M32(entity, 0x18) -= 0x100;
    M16(entity, 0x24) += 0x400;
    M16(entity, 0x26) += 0x300;
    sprite_draw_child(entity, 0x8000, 1);  /* braf tail-branch */
}

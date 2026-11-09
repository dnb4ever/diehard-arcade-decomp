/* effect_spin_rise_task [0x0223eef0]: Existing map corroborated by source: spinning rise: ++unk6C vs unk70, anim unk30, base pos g_06097E08, nudge unk10, tail func_060193f6 */
#include "dh_work.h"
#include "dh.h"
/* 0x0223eef0 (task handler) - spinning rise effect. entity = task->unk8.
 * ++entity->unk6C; kill when (unk6C-1) >= entity->unk70. Animate unk30 =
 * (unk6E & 3) + 0x1AC; load base pos from g_06097E08[unk34 << 8]; nudge unk10 by
 * +0x4000 if unk64 else -0x4000; tail physics func_022153f6(entity, 0x80000, 1).
 * fidelity: HIGH - unk6C/unk70/unk64/pos long, unk6E/unk30 word, unk34 byte.
 * braf cart 0x2153f6 -> RAM 0x060193f6. */
extern u8 g_06097e08[];
extern void sprite_draw_child(void *entity, s32, s32);

void effect_spin_rise_task(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 c = M32(ent, 0x6C) + 1;
    u8 *src;
    M32(ent, 0x6C) = c;
    if (c - 1 >= M32(ent, 0x70)) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M16(ent, 0x30) = (s16)((M16(ent, 0x6E) & 3) + 0x1AC);
    src = &g_06097e08[M8(ent, 0x34) << 8];
    M32(ent, 0x8)  = M32(src, 0);
    M32(ent, 0xC)  = M32(src, 4);
    M32(ent, 0x10) = M32(src, 8);
    M32(ent, 0x10) += (M32(ent, 0x64) != 0) ? 0x4000 : -0x4000;
    sprite_draw_child(ent, 0x80000, 1);
}

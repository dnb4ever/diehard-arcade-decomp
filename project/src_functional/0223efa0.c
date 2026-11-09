/* effect_tick_eight_frame_owner_flash [0x0223efa0]: Eight frame IDs from 0x188, position read from owner transform table. */
#include "dh_work.h"
#include "dh.h"
/* 0x0223efa0 (actor FSM) - 8-frame scripted action loading a pose vector from a table. Counter
 * at entity+0x6C; once it reaches 8 clear the task (+0x0) and return. While active set pose
 * field +0x30 = frame + 0x188 and load position (+0x8/+0xC/+0x10) from the 0x100-byte record
 * g_06097E08[entity type +0x34], then tail-branch (braf) to func_022153f6(entity, 0x80000, 1).
 * fidelity: HIGH - counter 32-bit, +0x30 16-bit, type +0x34 8-bit (record stride 0x100);
 * braf target resolved via braf_resolve.py (delta 0xfffd640e -> 0x02153f6). */
extern s8   g_06097E08[];    /* 0x100-byte pose records indexed by entity type */
extern void sprite_draw_child(void *entity, s32 a, s32 b);
void effect_tick_eight_frame_owner_flash(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 frame = M32(entity, 0x6C) + 1;
    M32(entity, 0x6C) = frame;
    frame -= 1;
    if (frame >= 8) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M16(entity, 0x30) = (s16)(frame + 0x188);
    {
        s8 *rec = &g_06097E08[M8(entity, 0x34) << 8];
        M32(entity, 0x8)  = M32(rec, 0x0);
        M32(entity, 0xC)  = M32(rec, 0x4);
        M32(entity, 0x10) = M32(rec, 0x8);
    }
    sprite_draw_child(entity, 0x80000, 1);  /* braf tail-branch */
}

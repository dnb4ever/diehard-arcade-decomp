/* particle_fall_task [0x022320d4]: Existing map corroborated by source: task->unk8=obj. Word life counter unk58 vs unk5A, or (src symbols_band23mid.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x022320d4 - particle/projectile task update. task->unk8 is the entity. Each
 * frame bumps a word life counter (unk58); when it passes the word limit (unk5A)
 * or global flag g_06098580 has bit 0x20000 set, the task is killed (task->unk0=0).
 * Otherwise a long frame counter (unk6C) advances and three fields are driven by
 * ashr_var (func_0224a420): the sprite attr/frame unk30 = (ashr(unk6C,g+1)&7)+0x188,
 * the x/accumulator unkC += ashr(0x0A00,g), and the depth unk70 += ashr(0x4000,g),
 * where g = g_06096c90. Finally tail-calls sprite_submit1(obj, obj->unk70, 1).
 * fidelity: HIGH - all 3 bsrf -> 0x0224a420 and tail braf -> 0x022153f6 confirmed. */
extern s32 g_scene_event_flags;
extern s32 g_06096c90;
extern s32 shc_shift_right_signed(s32 value, s32 count);        /* ashr_var */
extern void sprite_draw_child(void *obj, s32 depth, s32 flag);

void particle_fall_task(void *task) {
    void *obj = TASK_PTR(task, work);
    s16 cnt = (s16)(M16(obj, 0x58) + 1);
    M16(obj, 0x58) = cnt;
    if (cnt > M16(obj, 0x5A) || (g_scene_event_flags & 0x20000)) {
        TASK_S32(task, handler) = 0;
        return;
    }
    M32(obj, 0x6C) += 1;
    M16(obj, 0x30) = (s16)((shc_shift_right_signed(M32(obj, 0x6C), g_06096c90 + 1) & 7) + 0x188);
    M32(obj, 0xC) += shc_shift_right_signed(0x0A00, g_06096c90);
    M32(obj, 0x70) += shc_shift_right_signed(0x4000, g_06096c90);
    sprite_draw_child(obj, M32(obj, 0x70), 1);
}

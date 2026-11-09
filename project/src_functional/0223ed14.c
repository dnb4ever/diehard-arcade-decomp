/* effect_tick_short_sprite [0x0223ed14]: Lifetime+6c/+70 advances frame then sprite draw with scale 0x80000. */
/* Round 4: 06042d4a targets 060193f6 with (entity,0x80000,1); C had called the audio queue. Evidence: work/decomp4/callee_missing.asm. */
#include "dh_work.h"
#include "dh.h"
/* func_0223ed14(task): advance a spawned sound/effect object (entity = task->unk8).
 * Increment entity->unk6C; once it reaches entity->unk70, clear task->unk0 (done) and
 * return. Otherwise set entity->unk30 = (entity->unk6E & 3) + 0x1AC and tail-push a
 * sound via func_02215f6c(entity, 0x80000, 1).
 * fidelity: HIGH - offsets/widths verified from asm; func_02215f6c = sound_queue_push
 *   (tail braf) resolved; it is passed the three explicitly-set registers r4/r5/r6. */
extern void sprite_draw_child(void *ent, s32 a, s32 b);   /* sound_queue_push */

void effect_tick_short_sprite(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 old = M32(ent, 0x6C);

    M32(ent, 0x6C) = old + 1;
    if (old >= M32(ent, 0x70)) {
        TASK_PTR(task, handler) = 0;
        return;
    }
    M16(ent, 0x30) = (M16(ent, 0x6E) & 3) + 0x1AC;
    sprite_draw_child(ent, 0x80000, 1);
}

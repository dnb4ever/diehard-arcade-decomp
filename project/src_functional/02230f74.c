/* effect_scatter_task [0x02230f74]: Existing map corroborated by source: rising/scatter: ++unk58 cap 0x18, spin anim unk30, drawlist (<4), unkC/unk70 motion, tail func_060193f6 */
#include "dh_work.h"
#include "dh.h"
/* 0x02230f74 (task handler) - rising/scatter effect step. entity = task->unk8.
 * ++entity->unk58 (word); kill if > 0x18. Animate: unk30 = ((++unk6C)>>1 & 7)+0x188.
 * For the first few frames (unk58 < 4) also push to drawlist (func_0222eea2).
 * Apply motion: unkC += 0xA00, unk70 += 0x4000; tail-call physics step
 * func_022153f6(entity, unk70, 1).
 * fidelity: HIGH - unk58 word; unk6C/unkC/unk70 long. bsrf 0x22eea2 -> func_0222eea2,
 * braf cart 0x2153f6 -> RAM 0x060193f6. */
extern void collision_queue_actor(void *entity);         /* drawlist_prepend */
extern void sprite_draw_child(void *entity, s32, s32);

void effect_scatter_task(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 c;
    M16(ent, 0x58) = (s16)(M16(ent, 0x58) + 1);
    if ((s16)M16(ent, 0x58) > 0x18) {
        TASK_S32(task, handler) = 0;
        return;
    }
    c = M32(ent, 0x6C) + 1;
    M32(ent, 0x6C) = c;
    M16(ent, 0x30) = (s16)(((c >> 1) & 7) + 0x188);
    if ((s16)M16(ent, 0x58) < 4)
        collision_queue_actor(ent);
    M32(ent, 0xC)  += 0xA00;
    M32(ent, 0x70) += 0x4000;
    sprite_draw_child(ent, M32(ent, 0x70), 1);
}

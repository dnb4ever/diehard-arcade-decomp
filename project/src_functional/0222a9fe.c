/* actor_anim_step_task [0x0222a9fe]: Existing map corroborated by source: task-form anim frame step: decrement unkC4, advance anim-script ptr unkD0 on expiry, transition task->unk0 to func_0222a5ca on end (unk3C==0x10) */
#include "dh_work.h"
#include "dh.h"
/* 0x0222a9fe (actor anim step, task form) - task handler: task->unk8 = entity. Runs
 * func_0222eba8(task), animation_update(task, entity), draw_actor_hier_model(entity). If this frame's
 * loop flag unk54&2 is set, decrement the frame timer unkC4; when it reaches -1 (was 0) advance
 * the anim-script pointer p = entity->unkD0 + 8: if the new frame's id (p->unk0) is negative,
 * request a state end (unk3C = 0x10); otherwise load the frame (unk5C = p->unk0,
 * unkC4 = p->unk2 - 1, unk26 = p->unk6 unless 0xFFFF) and store the advanced pointer to unkD0.
 * Then, if the global anim channel g_06096C94 has its loop bit (unk4&2), mirror the current
 * frame's unk4 into g_06096C94.unk8. Finally, if a state end was requested (unk3C==0x10), run
 * func_02227c22(entity) and transition task->unk0 to func_0222a5ca.
 * fidelity: HIGH - unk54 word (unsigned bit test), unkC4/3C long, p/g_06096C94 fields word;
 * timer decrement-then-advance pattern; next-state pointer resolved from the PIC mova anchor
 * (0x22aa90 - 0x4C6 = func_0222a5ca). computed calls resolved via braf_rewrite.py. */
extern void animation_cell_tick(void *task);
extern void actor_anim_frame_update(void *ctx, void *entity);   /* animation_update */
extern void draw_actor_hier_model(void *entity);
extern void actor_release_slot(void *entity);
extern void task_clear_handler(void *task);
extern u8   g_06096C94[];

void actor_anim_step_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *p = ACTOR_MP(entity, animation_script);

    animation_cell_tick(task);
    actor_anim_frame_update(task, entity);
    draw_actor_hier_model(entity);
    if (ACTOR_MU16(entity, animation_events) & 2) {
        s32 c = M32(entity, 0xC4) - 1;
        M32(entity, 0xC4) = c;
        if (c == -1) {
            p = (char *)p + 8;
            if (M16(p, 0) < 0) {
                M32(entity, 0x3C) = 0x10;
            } else {
                M16(entity, ACTOR_ANIM_REQUEST) = M16(p, 0);
                M32(entity, 0xC4) = M16(p, 2) - 1;
                if (MU16(p, 6) != 0xFFFF)
                    M16(entity, OBJ_ANGLE_Y) = M16(p, 6);
                ACTOR_MP(entity, animation_script) = p;
            }
        }
    }
    if (MU16(g_06096C94, 4) & 2)
        M16(g_06096C94, 8) = M16(p, 4);
    if (M32(entity, 0x3C) == 0x10) {
        actor_release_slot(entity);
        TASK_PTR(task, handler) = (void *)task_clear_handler;
    }
}

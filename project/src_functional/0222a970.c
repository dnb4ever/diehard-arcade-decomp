/* actor_anim_init_task [0x0222a970]: Existing map corroborated by source: task-form anim init (task->unk8=entity): loads unkD0 anim rec, zeroes affine fields, primes unkC4 timer, installs func_0222a9fe as task->unk0 (next state) */
#include "dh_work.h"
#include "dh.h"
/* 0x0222a970 (actor anim init, task form) - task handler: task->unk8 = entity, task->unk0 =
 * next state handler. Loads the anim-script record entity->unkD0, zeroes the sprite affine
 * fields (unk20/22/24/28), sets angle unk26 = rec->unk6, latches state flag unk84=0x80000000,
 * primes frame timer unkC4 = rec->unk2 - 1, then runs func_0222db44(entity, rec->unk0) and
 * func_0221707c(entity,0,0,0,0), sets anim id unk5C = rec->unk0 and unk41=1, ticks
 * animation_update(task, entity), writes rec->unk4 to the global g_06096C9C, runs
 * func_0222eb84(entity), immediately runs the next handler func_0222a9fe(task) and installs it
 * as task->unk0.
 * fidelity: HIGH control flow / MED arg-arity on the opaque helpers (func_0222db44/0221707c/
 * 0222eb84); rec fields word, unk20-28/26/5C word, unk84/C4 long, unk41 byte; next-state
 * pointer resolved from the PIC mova anchor (0x22a9ec + 0x12 = func_0222a9fe).
 * computed calls resolved via braf_rewrite.py. */
extern void actor_load_pose_buffers(void *entity, s16 anim);
extern void actor_register_movement_slot(void *entity, s32 a, s32 b, s32 c, s32 d);
extern void actor_anim_frame_update(void *ctx, void *entity);   /* animation_update */
extern void animation_cell_init(void *entity);
extern void actor_anim_step_task(void *task);
extern s16  g_06096C9C;

void actor_anim_init_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *rec = ACTOR_MP(entity, animation_script);

    M16(entity, OBJ_ANGLE_Y) = M16(rec, 6);
    ACTOR_M16(entity, angle_z) = 0;
    ACTOR_M16(entity, angle_x) = 0;
    M16(entity, 0x20) = 0;
    M16(entity, 0x22) = 0;
    ACTOR_M32(entity, action_category) = (s32)0x80000000;
    M32(entity, 0xC4) = M16(rec, 2) - 1;
    actor_load_pose_buffers(entity, M16(rec, 0));
    actor_register_movement_slot(entity, 0, 0, 0, 0);
    M16(entity, ACTOR_ANIM_REQUEST) = M16(rec, 0);
    ACTOR_M8(entity, animation_control)  = DH_ANIMATION_CONTROL_RESTART;
    actor_anim_frame_update(task, entity);
    g_06096C9C = M16(rec, 4);
    animation_cell_init(entity);
    actor_anim_step_task(task);
    TASK_PTR(task, handler) = (void *)actor_anim_step_task;
}

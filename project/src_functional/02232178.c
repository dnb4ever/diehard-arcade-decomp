/* muzzle_spark_task [0x02232178]: Existing map corroborated by source: camera-anchored spark attached to actor; slot alloc func_02228106, orient func_0221080a, pos = cam g_06098B1C + src; tail func_0222eea2 */
#include "dh_work.h"
#include "dh.h"
/* 0x02232178 (effect task handler) - camera-anchored spark/muzzle flash attached
 * to an actor, (re)positioned each frame relative to the camera.
 * ent = task->unk8; src = task->unk14->unk8 (owner actor).
 * First frame only (task->unk10 == 0): seed ent->unk8/C/10 from src pos; allocate
 *   a draw slot func_02228106() -> ent->unk34; ent->unk60 = 4, unk62 = 29,
 *   unk64 = 0x3333; task->unk10++.
 * If ent->unk5D != 0: clear it and, when g_060e0ddc != 0, fire sound
 *   sound_queue_push(0xA01404FF).
 * ++ent->unk5A; every 16th frame ((unk5A & 0xF) == 0) clear ent->unk68.
 * ent->unk28 = src->unk28; orient via mtx_push_rotate_apply(g_0608807c, &ent->unk24).
 * World pos = camera g_06098b1c[0..2] + src->unk8/C/10; save prev pos
 *   (unk4C/50/54); ent->unk54 += 0x8000.
 * If g_06097410 != 0 kill task (task->unk0 = 0). Otherwise, when
 *   src->unk60 > 0x80 (long), tail-call func_0222eea2(ent) (drawlist).
 * fidelity: HIGH. bsrf resolved: 0x228106, 0x215f6c, 0x21080a; braf tail 0x22eea2. */
extern s32 g_transformed_vector[];
extern u8  g_0608807c[];
extern s32 g_sound_effects_enabled;
extern s32 g_06097410;
extern s32  actor_allocate_effect_slot(void);
extern void sound_queue_push(s32 cmd);
extern void mtx_push_rotate_apply(void *vel, void *ang);
extern void collision_queue_actor(void *entity);

void muzzle_spark_task(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);

    if (TASK_S8(task, state) == 0) {
        M32(ent, 0x8)  = M32(src, 0x8);
        M32(ent, 0xC)  = M32(src, 0xC);
        M32(ent, 0x10) = M32(src, 0x10);
        M8(ent, 0x34) = (s8)actor_allocate_effect_slot();
        M16(ent, 0x60) = 4;
        M16(ent, 0x62) = 29;
        M32(ent, 0x64) = 0x3333;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }

    if (M8(ent, 0x5D) != 0) {
        M8(ent, 0x5D) = 0;
        if (g_sound_effects_enabled != 0)
            sound_queue_push(0xA01404FF);
    }

    M16(ent, 0x5A) = (s16)(M16(ent, 0x5A) + 1);
    if ((M16(ent, 0x5A) & 0xF) == 0)
        M32(ent, 0x68) = 0;

    M16(ent, 0x28) = M16(src, 0x28);
    mtx_push_rotate_apply(g_0608807c, (void *)((u8 *)ent + 0x24));

    M32(ent, 0x8)  = g_transformed_vector[0] + M32(src, 0x8);
    M32(ent, 0xC)  = g_transformed_vector[1] + M32(src, 0xC);
    M32(ent, 0x10) = g_transformed_vector[2] + M32(src, 0x10);
    M32(ent, 0x4C) = M32(ent, 0x8);
    M32(ent, 0x50) = M32(ent, 0xC);
    M32(ent, 0x54) = M32(ent, 0x10);
    M32(ent, 0x54) += 0x8000;

    if (g_06097410 != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }
    if (M32(src, 0x60) > 0x80)
        collision_queue_actor(ent);
}

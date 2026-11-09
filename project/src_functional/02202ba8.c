/* scene_prop_gravity_task [0x02202ba8]: Spins and integrates a launched composite prop under gravity. */
#include "dh_work.h"
#include "dh.h"
/* 0x02202ba8 - task handler: a thrown/launched object with gravity (the briefcase / grenade
 * arc). entity = task->unk8; task->unk10 is the state byte.
 * state 0 (init): seed position (unk8/C/10 = 0x270000/0x80000/0x40000), zero velocity
 *   (unk14/18 = 0, unk1C = -0x1400 downward), spin (unk24=0x800, unk26=0x8800, unk28=0),
 *   flags unk37=unk36=1; if sound enabled (g_060e0ddc != 0) sound_queue_push(0xa01400ff);
 *   then task->unk10++.
 * state !=0: unk24 -= 0x800; mtx_push_rotate_apply(&entity->unk14, &entity->unk24) (orient/rotate step);
 *   unk24 += 0x800; integrate a world vector g_06098b1c[0..2] into position (unk8/C/10);
 *   tail func_0221b330(entity).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. unk1C/unk26 are the sign-extended pool shorts
 * 0xEC00/0x8800; unk24 is s16; braf tail -> 0x0221b330; bsrf -> 0x0221080a / sound 0x0215f6c. */
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern void sound_queue_push(s32 req);      /* 0x0215f6c */
extern void mtx_push_rotate_apply(void *vel, void *ang);
extern void model_draw_composite_parts(void *entity);

void scene_prop_gravity_task(void *task) {
    void *e = TASK_PTR(task, work);

    if (TASK_S8(task, state) == 0) {
        M32(e, 0x08) = 0x270000;
        M32(e, 0x0c) = 0x80000;
        M32(e, 0x10) = 0x40000;
        M32(e, 0x14) = 0;
        M32(e, 0x18) = 0;
        M32(e, 0x1c) = -0x1400;
        M16(e, 0x24) = 0x800;
        M16(e, 0x26) = (s16)0x8800;
        M16(e, 0x28) = 0;
        if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push((s32)DH_SOUND_PROP_LAUNCH_COMMAND);
        M8(e, 0x37) = 1;
        M8(e, 0x36) = 1;
        TASK_S8(task, state) += 1;
        return;
    }

    M16(e, 0x24) -= 0x800;
    mtx_push_rotate_apply((u8 *)e + 20, (u8 *)e + 36);
    M16(e, 0x24) += 0x800;
    M32(e, 0x08) += G32(DH_TRANSFORMED_VECTOR_ADDR);
    M32(e, 0x0c) += G32(0x06098b20);
    M32(e, 0x10) += G32(0x06098b24);
    model_draw_composite_parts(e);
}

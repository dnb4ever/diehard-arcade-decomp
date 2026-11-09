/* enemy_ground_slam_task [0x0223e9d2]: Runs the four-phase timed charge and ground-slam effect sequence. */
#include "dh_work.h"
#include "dh.h"
/* func_0223e9d2(task): 4-state charge/ground-slam sequence; entity = task->unk8, state
 * in entity->unkD4.
 *   0: init (unkC4=0x1E timer, unk98=0), fall through to 1.
 *   1: func_0223ec44; when timer unkC4 decrements past 0, reset unkC8 and advance;
 *      tail func_022462bc(entity).
 *   2: func_0223ec44 + func_0223eb3c; at tick unkC8==0x38 spawn a child via
 *      func_0220ff0a(task, template 0x02212350) and prime its entity, then clamp
 *      unkC8=0x38 and advance; tail func_022462bc(entity).
 *   3: func_0223eb3c; after 0x42 ticks drain g_060973E0 by entity->unkF8 (floored at 0),
 *      then func_022170b4 + func_02227c22 and clear task->unk0.
 * fidelity: HIGH - all offsets/field widths verified from asm; func_0220ff0a and the
 *   0x02212350 template (mova+add) resolved; siblings func_0223ec44/func_0223eb3c. */
extern void *task_spawn_actor_work(void *task, void *template);
extern void collision_release_slot_raw_entry(void *ent);
extern void actor_release_slot(void *ent);
extern void actor_draw_by_model_mode(void *ent);
extern void effect_spawn_jittered_spark(void *task, void *ent);
extern void effect_tick_jittered_emitter(void *task, void *ent);

void enemy_ground_slam_task(void *task) {
    void *ent = TASK_PTR(task, work);
    s32 t;

    switch (M32(ent, 0xD4)) {
    case 0:
        M32(ent, 0xC4) = 0x1E;
        M32(ent, 0x98) = 0;
        M32(ent, 0xD4) += 1;
        /* fallthrough */
    case 1:
        effect_tick_jittered_emitter(task, ent);
        t = M32(ent, 0xC4) - 1;
        M32(ent, 0xC4) = t;
        if ((t + 1) <= 0) {
            M32(ent, 0xC8) = 0;
            M32(ent, 0xD4) += 1;
        }
        actor_draw_by_model_mode(ent);
        return;
    case 2: {
        void *obj, *oe;
        effect_tick_jittered_emitter(task, ent);
        effect_spawn_jittered_spark(task, ent);
        t = M32(ent, 0xC8) + 1;
        M32(ent, 0xC8) = t;
        if ((t - 1) == 0x38) {
            obj = task_spawn_actor_work(task, (void *)0x02212350);
            M8(obj, 0xF) = 0x10;
            oe = MP(obj, 8);
            M32(oe, 8) = M32(ent, 8);
            M32(oe, 0xC) = M32(ent, 0xC);
            M32(oe, 0x10) = M32(ent, 0x10);
            M8(oe, 0x48) = 1;
            M8(oe, 0x4A) = M8(ent, 0x34);
            M16(oe, 0xB8) = 0;
            M32(oe, 0xB4) = 0x400;
            M16(oe, 0xBA) = 0;
            M8(oe, 0x49) = 0;
            M16(oe, 0x64) = 0x1A;
            M32(oe, 0x74) = 0x3000;
            M16(oe, 0x60) = 0x4000;
            M16(oe, 0x62) = 0;
            M32(ent, 0xC8) = 0x38;
            M32(ent, 0xD4) += 1;
        }
        actor_draw_by_model_mode(ent);
        return;
    }
    case 3:
        effect_spawn_jittered_spark(task, ent);
        t = M32(ent, 0xC8) + 1;
        M32(ent, 0xC8) = t;
        if ((u32)(t - 1) >= 0x42U) {
            s32 pool = M32((void *)0x060973E0, 0) - M32(ent, 0xF8);
            M32((void *)0x060973E0, 0) = pool;
            if (pool < 0) {
                M32((void *)0x060973E0, 0) = 0;
            }
            collision_release_slot_raw_entry(ent);
            actor_release_slot(ent);
            TASK_S32(task, handler) = 0;
        }
        return;
    }
}

/* enemy_enter_despawn [0x02249cda]: Installs the state-specific retract task, primes lifetime and releases the actor registry slot. */
#include "dh_work.h"
#include "dh.h"
/* 0x02249cda  actor_enter_despawn: install the despawn/retract task handler and
 * prime the actor for it. ABI (r4=task, r5=entity).
 *
 *   if (entity[0xe4] == 9) { task->fn = func_0223e9d2; state = 0; }
 *   else                   { task->fn = func_02249da8;      state = 2; }
 *   entity[0xd4] = state;
 *   entity[0xc4] = 32;
 *   entity[0xbc] = -1;
 *   entity[0xe0] |= 0x800;
 *   entity[0x84] |= 0x1000;
 *   b = task[0xf] & 0xc0;                   (top 2 bits of a task flag byte)
 *   if (b != 0 && (entity[0xe0] & 0xa000) && *(s32*)0x0609958c > 1)
 *       func_0223ab5c(entity, (b >> 6) & 3);
 *   release_slot_entry(entity);                  (braf tail, resolved)
 *
 * Both next-handler pointers resolved with ptr_resolve (mova+add): task->fn (the
 * task's own function slot at offset 0) is set to func_02249da8 (0x02249da8, the
 * 4-state retract task in this unit) or to func_0223e9d2 (0x0223e9d2). The trailing
 * jump-table-looking bytes m2c choked on are that resolved pointer's pool/data, not
 * code. fidelity: HIGH - hand-traced vs m2c_fn --asm. */
extern void enemy_retract_task(void *task);           /* 0x02249da8 */
extern void enemy_ground_slam_task(void *task);      /* 0x0223e9d2 */
extern void actor_spawn_secondary_effect(void *entity, s32 sel);
extern void release_slot_entry(void *entity);

void enemy_enter_despawn(void *task, void *entity) {
    s32 state;
    s32 b;

    if (ACTOR_M32(entity, state_index) == 9) {
        TASK_PTR(task, handler) = (void *)enemy_ground_slam_task;
        state = 0;
    } else {
        TASK_PTR(task, handler) = (void *)enemy_retract_task;
        state = 2;
    }
    ACTOR_M32(entity, behavior_state) = state;
    M32(entity, 0xC4) = 32;
    M32(entity, 0xBC) = -1;
    ACTOR_M32(entity, state_flags) |= 0x800;
    ACTOR_M32(entity, action_category) |= 0x1000;

    b = M8(task, 0xF) & 0xC0;
    if (b != 0 && (ACTOR_M32(entity, state_flags) & 0xA000) && *(s32 *)(uintptr_t)0x0609958C > 1) {
        actor_spawn_secondary_effect(entity, (b >> 6) & 3);
    }
    release_slot_entry(entity);
}

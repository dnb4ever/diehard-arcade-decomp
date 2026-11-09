/* enemy_tick_state_table_c [0x02241c40]: Third state-table task with same behavior-17 interrupt and draw/grid sequence. */
#include "dh_work.h"
#include "dh.h"
/* 0x02241c40 - func_02241c40(entity): state-dispatch step for an actor.
 * a = entity->0x08 (actor). If func_02245dcc(a) != 0, enter state 0x11:
 * a->0xD4 = 0x11; a->0xD8 = func_0223ffcc((s32)entity, a).
 * Then dispatch: t = *(u32*)(0x060901C4 + a->0xD4*4); t(entity);
 * draw_actor_hier_shadowed(a); tail-call func_0222eee4(a).
 * fidelity: HIGH - the braf delay slot at 0x241c86 (mov r14,r4) passes the
 * ACTOR to func_0222eee4, not the entity; the loaded dispatch word is
 * spilled to a stack slot (dead) before the indirect call. */

typedef void (*fnptr_entity)(void *entity);

s32 enemy_attack_is_armed(void *a);
s32 actor_dispatch_action_and_finish(s32 arg0, void *entity);
void draw_actor_hier_shadowed(void *actor);
void collision_insert_grid(void *entity);

void enemy_tick_state_table_c(void *entity)
{
    void *a = (void *)(uintptr_t)MU32((u8 *)(uintptr_t)entity, 8);
    u8 *tbl = (u8 *)(uintptr_t)0x060901C4;
    s32 t;

    if (enemy_attack_is_armed(a) != 0) {
        ACTOR_M32(a, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
        ACTOR_M32(a, behavior_phase) = actor_dispatch_action_and_finish((s32)(uintptr_t)entity, a);
    }

    t = (s32)MU32(tbl, (u32)ACTOR_M32(a, behavior_state) * 4);
    ((fnptr_entity)(uintptr_t)t)(entity);

    draw_actor_hier_shadowed(a);
    collision_insert_grid(a);
}

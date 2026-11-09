/* enemy_tick_state_table_a [0x02241b9c]: Forced-hit gate selects behavior 17; dispatch table 060901c4 then draw/grid. */
#include "dh_work.h"
#include "dh.h"
/* 0x02241b9c - func_02241b9c(entity): state-dispatch step for an actor.
 * a = entity->0x08 (actor). If func_02245dcc(a) != 0, enter state 0x11:
 * a->0xD4 = 0x11; a->0xD8 = func_0223ffcc((s32)entity, a).
 * Then dispatch: t = *(u32*)(0x060901C4 + a->0xD4*4); t(entity);
 * draw_actor_hier_shadowed(a); tail-call func_0222eee4(a).
 * NOTE: the braf tail call's delay slot (0x241be2 `mov r14,r4`) passes the
 * ACTOR (r14), not the entity - unlike sibling 02241bee/02241c40.
 * fidelity: HIGH - preserves call/store ordering; the handler table lives at
 * runtime address 0x060901C4 and is indexed by the (re)loaded 0xD4 field. */

typedef void (*fnptr_entity)(void *entity);

s32 enemy_attack_is_armed(void *a);
s32 actor_dispatch_action_and_finish(s32 arg0, void *entity);
void draw_actor_hier_shadowed(void *actor);
void collision_insert_grid(void *entity);

void enemy_tick_state_table_a(void *entity)
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

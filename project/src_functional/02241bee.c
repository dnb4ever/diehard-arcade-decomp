/* enemy_tick_state_table_b [0x02241bee]: Separate copy of forced-hit/table dispatch with shadowed draw and collision registration. */
#include "dh_work.h"
#include "dh.h"
/* 0x02241bee  func_02241bee(entity): dispatch through the actor state table.
 *   actor = entity->0x08 (pointer);
 *   if (func_02245dcc(actor) != 0) {
 *       actor->0xD4 = 17;
 *       actor->0xD8 = func_0223ffcc(entity, actor);
 *   }
 *   state = actor->0xD4;
 *   fn = *(u32*)(0x060901c4 + state*4);   (runtime table of handlers)
 *   fn(entity);
 *   draw_actor_hier_shadowed(actor);
 *   func_0222eee4(actor);   (tail call passes the ACTOR: 0x241c32 mov r14,r4)
 * fidelity: HIGH - preserves call/store ordering; the handler table is
 * indexed by state*4 bytes (shll2 at 0x241c1e); both r14 uses (draw at
 * 0x241c30 and the braf tail at 0x241c32) pass the actor, not the entity. */

typedef void (*fnptr_entity)(void *entity);

s32 enemy_attack_is_armed(void *a);
s32 actor_dispatch_action_and_finish(s32 arg0, void *entity);
void draw_actor_hier_shadowed(void *actor);
void collision_insert_grid(void *entity);

void enemy_tick_state_table_b(void *entity)
{
    void *actor = (void *)(uintptr_t)MU32((u8 *)(uintptr_t)entity, 8);
    u8 *tbl = (u8 *)(uintptr_t)0x060901C4;
    s32 t;

    if (enemy_attack_is_armed(actor) != 0) {
        ACTOR_M32(actor, behavior_state) = DH_ENEMY_BEHAVIOR_REACTION;
        ACTOR_M32(actor, behavior_phase) = actor_dispatch_action_and_finish((s32)(uintptr_t)entity, actor);
    }

    t = (s32)MU32(tbl, (u32)ACTOR_M32(actor, behavior_state) * 4);
    ((fnptr_entity)(uintptr_t)t)(entity);

    draw_actor_hier_shadowed(actor);
    collision_insert_grid(actor);
}

/* enemy_animate_category_b [0x0223f23c]: Handles clash or hit before dispatching the second variant table and animation. */
#include "dh.h"
/* 0x0223f23c - func_0223f23c(task, entity): per-tick combat dispatch variant
 * (sibling of actor_tick_dispatch 0x02249f52). Picks one of three reactions
 * then runs the shared animation_update:
 *   if (actor_flag_rng_1in4(entity))       actor_enter_clash(entity);
 *   else if (entity->0x4C)                 func_022489d0(entity);
 *   else                                   g_0608d7ec[entity->0x6D](entity);
 *   animation_update(task, entity);        // jsr @r12
 * fidelity: HIGH - all branches resolve; r13=task(arg0), r14=entity(arg1).
 * entity->0x4C and entity->0x6D read as signed bytes (mov.b); 0x6D indexes a
 * fn-ptr table. The animation_update address (0x0222dc2c) is formed by mova+add
 * into r12 at the prologue and called last. The fn-ptr spill to stack is a
 * scratch artifact and is omitted. */
typedef void (*react_handler)(void *entity);

extern s32  actor_flag_rng_1in4_entry(void *entity);                   /* 0x02246208 actor_flag_rng_1in4 */
extern void enemy_enter_clash(void *entity);                   /* 0x02249a82 actor_enter_clash */
extern void actor_enter_hit_reaction(void *entity);                   /* 0x022489d0 */
extern void actor_anim_frame_update_entry(void *task, void *entity);       /* 0x0222dc2c animation_update */
extern react_handler g_0608d7ec[];                         /* react table, idx entity->0x6D */

void enemy_animate_category_b(void *task, void *entity)
{
    if (actor_flag_rng_1in4_entry(entity) != 0)
        enemy_enter_clash(entity);
    else if (M8(entity, 0x4C) != 0)
        actor_enter_hit_reaction(entity);
    else
        g_0608d7ec[M8(entity, 0x6D)](entity);

    actor_anim_frame_update_entry(task, entity);
}

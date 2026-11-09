/* enemy_update_category_b_task [0x0223f10e]: Dispatches the second actor category's behavior state, draw and grid continuation. */
#include "dh_work.h"
#include "dh.h"

/* 0x0223f10e - one state handler in the func_0223f2xx dispatch family.
 *
 * MIRAGE CORRECTION: fn.py/ bandbig23 measured the whole pool-unit
 * [0x23f032,0x24003e) (0xaf8 / 46 branches) as "cmd_router_b".  The REAL
 * func_0223f10e is only [0x0223f10e, 0x0223f160)  (~40 insn); it ends at the
 * guarded tail-call 'braf -> func_0222eee4' at 0x23f15c.  The unit holds a run
 * of siblings: func_0223f198, func_0223f1ea, func_0223f23c (near-identical
 * state handlers), func_0223f29a (the real large flag-gated router - shipped
 * separately as 0223f29a.c), func_0223f494, func_0223f54e, ...  The "46
 * branches" and the phantom table @0x23f3dc (mask 0x0003FFFF) are unit-wide.
 *
 * arg0 = task; ent = task->8.  If func_02245dcc(ent): set ent->0xD4 = 17 and
 * ent->0xD8 = func_0223ffcc(task, ent).  Then dispatch through the per-state
 * handler table g_0608D6C4[ent->0xD4](task, ent), run func_0222ca98(ent), and
 * tail-call func_0222eee4(ent).
 *
 * m2c: all-M2C_ERROR for this TU; hand-traced.  The 'mov.l r2,@r15' at 0x23f144
 * is a dead spill of the function pointer (callee does not read the stack slot;
 * the function reserves no other stack and ends in a tail call).
 * fidelity: HIGH. */

s32  enemy_attack_is_armed(void *ent);                   /* 0x02245dcc */
s32  actor_dispatch_action_and_finish(void *task, void *ent);        /* 0x0223ffcc */
void draw_actor_hier_shadowed_entry(void *ent);                   /* 0x0222ca98 */
void collision_insert_grid(void *ent);                   /* 0x0222eee4 */

void enemy_update_category_b_task(void *task)
{
    void *ent = TASK_PTR(task, work);

    if (enemy_attack_is_armed(ent) != 0) {
        M32(ent, 0xD4) = DH_ENEMY_BEHAVIOR_REACTION;
        M32(ent, 0xD8) = actor_dispatch_action_and_finish(task, ent);
    }

    /* per-state handler table g_0608D6C4[ent->0xD4](task, ent) */
    (*(void (**)(void *, void *))(uintptr_t)(0x0608D6C4 + (M32(ent, 0xD4) << 2)))
        (task, ent);

    draw_actor_hier_shadowed_entry(ent);
    collision_insert_grid(ent);                    /* tail */
}

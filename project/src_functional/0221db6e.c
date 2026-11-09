/* prop_tick_attached_marker [0x0221db6e]: Tracks live object 060973f8; model 0x28, copied slot and biased X. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221db6e (actor FSM) - 3-state task handler on the task node's state byte (task+0x10),
 * driving the entity record at task+0x8. state 0: set entity+0x30 = 0x28, and if the global
 * object g_060973F8 is live copy its type byte (+0x34) into the entity and advance state.
 * state 1: set entity+0x8 from g_060973F8+0x8 (biased by 0xFFFE8FE6), and advance state only
 * while the active-slot flag for this entity's type is clear. state 2: clear the task (+0x0)
 * and return. All non-terminal states tail-branch (braf) to func_02246282(entity).
 * fidelity: HIGH - field widths from the loads (state/type 8-bit, +0x30 16-bit, +0x8 32-bit);
 * braf target resolved via tools/decomp/braf_resolve.py (delta 0x286aa -> 0x02246282). */
extern void *g_060973F8;      /* current global object (nullable) */
extern s32   g_actor_slot_ids[];    /* per-type active-slot flags */
extern void  actor_draw_transformed(void *entity);

void prop_tick_attached_marker(void *task) {
    void *entity = TASK_PTR(task, work);
    s32 state = TASK_S8(task, state);
    switch (state) {
    case 0:
        M16(entity, 0x30) = 0x28;
        if (g_060973F8 != 0) {
            M8(entity, 0x34) = M8(g_060973F8, 0x34);
            TASK_S8(task, state) = (s8)(state + 1);
        }
        break;
    case 1:
        M32(entity, 0x8) = M32(g_060973F8, 0x8) + (s32)0xFFFE8FE6;
        if (g_actor_slot_ids[M8(entity, 0x34)] == 0)
            TASK_S8(task, state) = (s8)(state + 1);
        break;
    case 2:
        TASK_S32(task, handler) = 0;
        return;
    default:
        break;
    }
    actor_draw_transformed(entity);  /* braf tail-branch */
}

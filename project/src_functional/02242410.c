/* enemy_tick_launch_wrapper [0x02242410]: Three task phases allocate collision slot and spawn in-flight handler 022424c4. */
#include "dh_work.h"
#include "dh.h"
/* 0x02242410  "thrown/launched" task stepper. arg0 = task wrapper,
 * inner = MP(task,8) is the actor. State machine on M8(task,0x10) (0..2):
 *   0 launch: set heading 0x474, snapshot x into unkEC, grab a collision slot
 *     (func_02228106 -> unk34), unk38 = 0x100000, register the in-flight handler
 *     func_022424c4 via func_0220ff0a, advance to 1.
 *   1 carry: if the active actor slot g_06099590[g_06097414] is live, ride its
 *     anchor g_060995b0[..]->unk8: when unkE0&0x40000 (landed) zero the motion
 *     accumulators unk14/18/1C and go to 2; else track x off the anchor's unkEC.
 *   2 done: nothing.
 * Always tail-calls func_02246282(inner) (shared sound/voice-task tail).
 *
 * bounds: [0x242410, 0x2424c4); split out of func_02242272's over-swallowed
 *   extent (prologue follows the braf-tail to func_02246282 at 0x2424c0; the
 *   next function func_022424c4 begins at 0x2424c4).
 * fidelity: HIGH - verified vs m2c/--asm; func_02228106 takes no args. */
extern s8   g_06097414;        /* current-actor playerIdx latch */
extern s32  g_actor_slot_ids[];      /* per-slot active flag */
extern void *g_actor_slot_tasks_legacy[];     /* per-slot anchor pointers; deref +8 -> actor */

s32  actor_allocate_effect_slot(void);                        /* 0x02228106 collision-slot grab */
void *task_spawn_actor_work(void *task, void *handler);  /* 0x0220ff0a register handler */
void enemy_tick_launched_body(void *task);                  /* 0x022424c4 in-flight handler */
void actor_draw_transformed(void *actor);                 /* 0x02246282 shared tail */

void enemy_tick_launch_wrapper(void *task) {
    void *inner = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0:
        M16(inner, 0x30) = 0x474;
        M32(inner, 0xEC) = M32(inner, 0x08);
        M8(inner, 0x34) = (s8)actor_allocate_effect_slot();
        M32(inner, 0x38) = 0x100000;
        task_spawn_actor_work(task, (void *)enemy_tick_launched_body);
        TASK_S8(task, state) += 1;
        break;
    case 1: {
        s8 slot = g_06097414;
        if (g_actor_slot_ids[slot] != 0) {
            void *anchor = TASK_PTR(g_actor_slot_tasks_legacy[slot], work);
            if (M32(inner, 0xE0) & 0x40000) {
                M32(inner, 0x14) = 0;
                M32(inner, 0x18) = 0;
                M32(inner, 0x1C) = 0;
                TASK_S8(task, state) = 2;
            } else if (M32(anchor, 0xE0) & 0x400) {
                M32(inner, 0x08) = (M32(inner, 0xEC) - M32(anchor, 0x08)) + M32(anchor, 0xEC);
            } else {
                M32(inner, 0x08) = M32(inner, 0xEC);
            }
        }
        break;
    }
    case 2:
        break;
    }

    actor_draw_transformed(inner);
}

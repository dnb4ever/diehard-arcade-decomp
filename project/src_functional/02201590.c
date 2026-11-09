/* boot_display_mode_task [0x02201590]: Allocates boot work and selects the boot director continuation from the startup mode. */
#include "dh_work.h"
#include "dh.h"
/* 0x02201590 - boot/display-mode state task. entity = task->unk8; task->unk10 is the state.
 * state 0: alloc_array_zeroed(task,1,0x6c); choose next state byte from table 0x0604ebd8
 *   indexed by g_060987cc; publish task into g_060987b8; then branch on g_060987cc:
 *     0 -> func_022058a8(task); install handler func_022016ac into *task; return.
 *     1 -> if ((g_06000798 >> 5) & 1) { g_060987d0 = 1; (*g_06000680)(1,1,0x6c); }
 *          else g_060987d0 = 0; install func_022016ac into *task; return.
 *     other -> continue into state-11 code.
 * state 11: func_0220fdc8(0); clear entity->unk3c bit0; spawn_obj_0x6c(task, func_02209188);
 *   g_060987c8 = 2; task->unk10 = 12; continue into state-12 code.
 * state 12: if (entity->unk3c & 1) { task->unk10 = 1; tail (*g_0600060c)(g_060987c8); }.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. bsrf -> 0x214bb8/0x2058a8/0x20fdc8/0x20fece;
 * mova+disp -> handlers 0x2016ac and 0x209188; g_06000798 shifted logically (>>5). The tail
 * BIOS call *g_0600060c sets only r4=g_060987c8; its r5/r6 are inherited/unmodeled (undefined
 * on the direct state-12 entry path), so only the one argument is modeled. */
#define G32(a)   (*(s32 *)(uintptr_t)(a))
#define GU32(a)  (*(u32 *)(uintptr_t)(a))
#define GP(a)    (*(void **)(uintptr_t)(a))

extern void *alloc_array_zeroed(void *pool, s32 count, s32 size); /* 0x214bb8 */
extern void  scene_bootstrap(void *task);
extern void  boot_scene_director_task(void *task);
extern void  results_task_dispatch(void *task);
extern void  task_cancel_below_depth(s32 arg);                 /* 0x20fdc8 */
extern void  spawn_obj_0x6c(void *parent, void *handler); /* 0x20fece */

void boot_display_mode_task(void *task) {
    void *entity = TASK_PTR(task, work);

    switch (TASK_S8(task, state)) {
    case 0: {
        s32 sel;
        alloc_array_zeroed(task, 1, 0x6c);
        entity = TASK_PTR(task, work);
        TASK_S8(task, state) = (s8)M32((uintptr_t)0x0604ebd8 + G32(0x060987cc) * 4, 0);
        GP(0x060987b8) = task;
        sel = G32(0x060987cc);
        if (sel == 0) {
            scene_bootstrap(task);
            TASK_PTR(task, handler) = (void *)boot_scene_director_task;
            return;
        }
        if (sel == 1) {
            if (((GU32(0x06000798) >> 5) & 1) == 0) {
                G32(0x060987d0) = 1;
                (*(void (**)(s32, s32, s32))(uintptr_t)0x06000680)(1, 1, 0x6c);
            } else {
                G32(0x060987d0) = 0;
            }
            TASK_PTR(task, handler) = (void *)boot_scene_director_task;
            return;
        }
        /* sel >= 2: fall through to state 11 */
    }
    /* fall through */
    case 11:
        task_cancel_below_depth(0);
        M32(entity, 0x3c) &= ~1;
        spawn_obj_0x6c(task, (void *)results_task_dispatch);
        G32(0x060987c8) = 2;
        TASK_S8(task, state) = 12;
        /* fall through */
    case 12:
        if (M32(entity, 0x3c) & 1) {
            TASK_S8(task, state) = 1;
            (*(void (**)(s32))(uintptr_t)0x0600060c)(G32(0x060987c8));
        }
        return;
    default:
        return;
    }
}

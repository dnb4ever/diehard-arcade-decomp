/* scene_prepare_record_bootstrap [0x02202d7e]: Clears player/HUD state and installs the record-bootstrap trampoline. */
#include "dh_work.h"
#include "dh.h"
/* 0x02202d7e - scene bring-up step of the 0x02202d14 trampoline family
 * (siblings 0x202d14, 0x202d7e, 0x202dda, 0x202dfa; shared literal pool at
 * 0x202e22..). Not called directly: reached as a task handler.
 *   func_0220f990();                            (r4 = task, unused by callee)
 *   g_06098689 = 0; g_06098688 = 0;             (u8, in that order)
 *   func_0223dcc2(1); func_0223dcc2(2);
 *   v = (s16)g_06099794; g_0609978A = v; g_06099788 = v;   (s16 pair)
 *   g_0609868D = 0; g_0609868C = 0;             (u8)
 *   g_06098580 = 0; g_0609856C = 0;             (s32)
 *   spawn_obj_0x6c(task, scene_hud_task_init);  (handler = mova 0x202dc0+0x64C = 0x20340c)
 *   task->unk0 = 0x06006dda;                    (mova 0x202dcc+0xE)
 *   tail (bra) func_02202dda(task).
 * fidelity: HIGH - hand-traced vs capstone 0x202d7e..0x202dd8; bsrf targets
 * 0x20f990, 0x23dcc2 (x2), 0x20fece resolved from the pool. */

#define G8(a)   (*(s8  *)(uintptr_t)(a))
#define G16(a)  (*(s16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern void  input_reset_records(void);                          /* 0x0220f990 */
extern void  player_clear_saved_action(s32 idx);                       /* 0x0223dcc2 */
extern void *spawn_obj_0x6c(void *parent, void *handler);  /* 0x0220fece */
extern void  hud_install_status_task(void *task);                    /* scene_hud_task_init */
void scene_bootstrap_record_dispatch(void *task);                            /* 0x02202dda */

void scene_prepare_record_bootstrap(void *task) {
    s16 v;

    input_reset_records();
    G8(0x06098689) = 0;
    G8(0x06098688) = 0;
    player_clear_saved_action(1);
    player_clear_saved_action(2);
    v = G16(0x06099794);
    G16(0x0609978A) = v;
    G16(0x06099788) = v;
    G8(0x0609868D) = 0;
    G8(0x0609868C) = 0;
    G32(DH_SCENE_EVENT_FLAGS_ADDR) = 0;
    G32(0x0609856C) = 0;
    spawn_obj_0x6c(task, (void *)hud_install_status_task);
    TASK_PTR(task, handler) = (void *)0x06006DDA;
    scene_bootstrap_record_dispatch(task);
}

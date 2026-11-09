/* evt_timed_dispatch [0x02233122]: Existing map corroborated by source: task step: init, countdown, then INDIRECT dispatch (*g_060902A0)(...), end */
#include "dh_work.h"
#include "dh.h"
/* 0x02233122 evt_timed_dispatch - task step: init once, count down, then fire a
 * global indirect dispatch and terminate.
 * ABI: (task, arg1, r6_passthrough, r7_passthrough) - r6/r7 are never written
 * before the jsr at 0x233168 and are forwarded verbatim to the indirect callee.
 *   disp_arg = arg1;
 *   state 0: disp_arg = &func_02233178;         (template, also passed to spawner)
 *            func_0220ff54(task, disp_arg);
 *            task->unkF = 0x5A;  task->unk10++.
 *   c = task->unkF;  task->unkF = c - 1;
 *   if (c <= 0) {                                 (timer expired)
 *       g_060997A4 = 0x11;  g_060997A8 = 3;
 *       (*g_060902A0)(task, disp_arg, r6_in, r7_in, g_06088824);
 *         The slot resolves to func_02223460 (varargs-style formatter): r4-r7
 *         are unused dummy register words and the format pointer 0x06088824 is
 *         the FIRST STACK argument, pushed by the jsr delay slot
 *         'mov.l r3,@-r15' at 0x23316a (popped at 0x23316c).
 *       task->unk0 = 0;
 *   }
 * fidelity: HIGH - bsrf resolved: func_0220ff54 (0x20ff54), template 0x233178
 * via mova(0x233134)+0x44. Countdown tests the pre-decrement s8 (exts.b/cmp/pl
 * on the value BEFORE the store, i.e. c+1 where c is the stored decremented
 * byte - equivalently the pre-decrement value <= 0). */
extern void  task_spawn_current_work40(void *task, void *template);
extern void  prop_tick_short_rotation(void);       /* dispatch template handler */
extern void (*g_060902A0)(void *, void *, s32, s32, void *); /* -> func_02223460 */
extern s32   g_ui_cursor_column;
extern s32   g_ui_cursor_row;
extern u8    g_06088824[];              /* format string for the dispatch */

void evt_timed_dispatch(void *task, void *arg1, s32 r6_in, s32 r7_in) {
    void *disp_arg = arg1;

    if (TASK_S8(task, state) == 0) {
        disp_arg = (void *)prop_tick_short_rotation;
        task_spawn_current_work40(task, disp_arg);
        M8(task, 0xF) = 0x5A;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);
    }
    {
        s8 c = M8(task, 0xF);
        M8(task, 0xF) = (s8)(c - 1);
        if (c <= 0) {
            g_ui_cursor_column = 0x11;
            g_ui_cursor_row = 3;
            g_060902A0(task, disp_arg, r6_in, r7_in, g_06088824);
            TASK_S32(task, handler) = 0;
        }
    }
}

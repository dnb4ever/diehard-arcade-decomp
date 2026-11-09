/* scene_initialize_hud_task [0x02202c5c]: Resets input, saved player state and HUD flags before installing the scene bootstrap. */
#include "dh_work.h"
#include "dh.h"
/* 0x02202c5c (task handler, scene/HUD setup) - task->unk8 is the owning task. Runs
 * func_0220f990(), clears the two status bytes at g_06098688, runs func_0223dcc2(1) and
 * func_0223dcc2(2), mirrors the language id g_06099794 into g_06099788 (+0 and +2), clears the
 * flag bytes g_0609868D/g_0609868C and g_06098580, spawns an object via
 * func_0220fece(task, template 0x0600740C), and sets task->unk0 to func_02202dfa
 * (runtime form 0x06006DFA).
 * fidelity: HIGH - g_06098688 bytes, g_06099788 words, g_06099794 word, g_0609868C/D bytes,
 * g_06098580 long. The mova PIC values are RUNTIME HWRAM addresses (code segment loaded at
 * 0x06004000 = cart 0x200000): mova(0x202c9c)+0x770 -> 0x0600740C data template;
 * mova(0x202cac)+0x14E -> 0x06006DFA (func_02202dfa entry). bsrf targets resolved:
 * func_0220f990 / func_0223dcc2 (x2) / func_0220fece. */
extern void input_reset_records(void);
extern void player_clear_saved_action(s32 arg);
extern void task_spawn_current_work6c(void *task, void *template);   /* spawn_obj_0x6c */
extern void scene_bootstrap_record_select(void *task);
extern u8   g_06098688[];
extern u8   g_06099788[];
extern s16  g_06099794;
extern u8   g_0609868C, g_0609868D;
extern s32  g_scene_event_flags;

void scene_initialize_hud_task(void *task) {
    s16 v;

    input_reset_records();
    M8(g_06098688, 1) = 0;
    M8(g_06098688, 0) = 0;
    player_clear_saved_action(1);
    player_clear_saved_action(2);
    v = g_06099794;
    M16(g_06099788, 2) = v;
    M16(g_06099788, 0) = v;
    g_0609868D = 0;
    g_0609868C = 0;
    g_scene_event_flags = 0;
    task_spawn_current_work6c(task, (void *)0x0600740C);
    TASK_PTR(task, handler) = (void *)0x06006DFA;
}

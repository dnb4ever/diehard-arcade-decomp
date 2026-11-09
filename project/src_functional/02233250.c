#include "dh_work.h"
/* scene_spawn_display_and_effects [0x02233250]: Builds scene backgrounds, spawns HUD managers and consumes scene task/effect lists. */
#include "dh.h"
/* 0x02233250 - per-scene stage setup + fade-list / particle spawner. From the
 * active scene descriptor g_0609866c->unk28 it indexes the fade-descriptor table
 * g_06088700 (stride 0xC). Runs the scene81 stage loader (scene81_stage_setup), the BG
 * layer builder (scene_bg_layer_build) and, when g_06098668 == 0x22, func_022078aa.
 * Spawns two fixed 0x6C-byte objects (templates @runtime 0x0603B1C0 / 0x0603BEA6,
 * symbol 0x022371C0 / 0x02237EA6). If the table entry has a spawn-record list (unk4)
 * it builds it via func_02233388; if it has a particle list (unk0) it tail-calls
 * func_022332d4 to emit them.
 * fidelity: HIGH - 5 bsrf + tail bra confirmed; templates resolved from mova+add
 * (0x2332a4+0x3F28 -> 0x023371C0, 0x2332a4... 0x2332a0+0x4C02 -> 0x02337EA6 symbol
 * space), expressed as runtime addresses (symbol + 0x03E04000). */
extern void *g_scene_descriptor_legacy;                             /* active scene descriptor */
extern u8    g_06088700[];                            /* fade-descriptor table, stride 0xC */
extern s32   g_scene_index;
extern void  scene81_stage_setup(void *task, void *desc);   /* scene81_stage_setup */
extern void  scene_bg_layer_build(void *task, void *desc);   /* scene_bg_layer_build */
extern void  scene_load_tiled_layer_with_task(void *task, void *desc);
extern void  task_spawn_current_work6c(void *task, void *template);   /* spawn_obj_0x6c */
extern void  scene_spawn_record_list(void *parent, void *record);   /* spawn-list builder */
extern void  scene_spawn_effect_list(void *node);                   /* particle-list emitter */

void scene_spawn_display_and_effects(void *task) {
    void *desc = g_scene_descriptor_legacy;
    s32 idx = (s16)(SCENE_M16(desc, pose_index) * 0xC);
    void *entry = (u8 *)g_06088700 + idx;
    void *record;
    void *head;

    scene81_stage_setup(task, desc);
    scene_bg_layer_build(task, desc);
    if (g_scene_index == 0x22)
        scene_load_tiled_layer_with_task(task, desc);
    task_spawn_current_work6c(task, (void *)0x0603B1C0);
    task_spawn_current_work6c(task, (void *)0x0603BEA6);

    record = MP(entry, 4);
    if (record != 0)
        scene_spawn_record_list(task, record);
    head = MP(entry, 0);
    if (head != 0)
        scene_spawn_effect_list(head);
}

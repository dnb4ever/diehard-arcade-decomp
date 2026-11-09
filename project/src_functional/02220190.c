/* spawn_entry_list [0x02220190]: Existing map corroborated by source: spawn one child task per entry of g_06090970[scene->unk28] (while entry->unk2 > 0) */
#include "dh_work.h"
#include "dh.h"
/* 0x02220190 spawn_entry_list - spawn a child task per entry of a scene spawn table.
 * arg0 = parent task.
 *   entry = g_06090970[(*g_0609866C)->unk28];     (per-scene entry list pointer)
 *   while (entry->unk2 > 0) {                       (s16 terminator/active field)
 *       child = func_0220fece(arg0, &func_022201d8);
 *       child->unk8->unk64 = entry;                 (attach the entry to the child entity)
 *       entry += 8;                                 (advance to next 8-byte entry)
 *   }
 * fidelity: HIGH - mova+add resolved: jsr target 0x20fece (child spawner), template
 * 0x022201d8; loop guard is a bottom-tested s16 at entry+2. g_06090970 is an array of
 * per-scene entry-list pointers indexed by scene->unk28. Matches m2c --asm. */
extern void *task_spawn_current_work6c(void *task, void *template);
extern void  display_tick_strip_upload(void);
extern void *g_scene_descriptor;        /* current scene descriptor pointer */
extern void *g_06090970[];      /* per-scene entry-list pointers */

void spawn_entry_list(void *task) {
    void *entry = g_06090970[SCENE_M16(g_scene_descriptor, pose_index)];

    while (M16(entry, 0x2) > 0) {
        void *child = task_spawn_current_work6c(task, (void *)display_tick_strip_upload);
        MP(MP(child, 0x8), 0x64) = entry;
        entry = (void *)((u8 *)entry + 8);
    }
}

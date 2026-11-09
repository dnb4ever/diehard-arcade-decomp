#include "dh_work.h"
/* scene_bootstrap_record_dispatch [0x02202dda]: Selects the scene's twelve-byte bootstrap record and delegates without changing selection flags. */
#include "dh.h"
/* 0x02202dda - per-scene trampoline of the 0x02202d14 family (installed as
 * task->unk0 by func_02202d7e, which also falls into it).
 * Mirror of func_02202dfa without the g_0609856c |= 1 side effect:
 *   scene = *0x0609866c; record = 0x06074550 + (s16)scene->unk28 * 12;
 *   tail (braf) func_02218038(task, record).
 * fidelity: HIGH - hand-traced vs capstone 0x202dda..0x202df8; braf delta
 * 0x0001523E (0x202dfa + 0x1523E = 0x218038). The 4-byte stack slot written
 * before the braf is a dead spill. */

#define GP(a)   (*(void **)(uintptr_t)(a))

extern void scene_record_bootstrap_task(void *task, void *record);   /* scene_record_bootstrap_sm */

void scene_bootstrap_record_dispatch(void *task) {
    void *scene = GP(DH_SCENE_DESCRIPTOR_ADDR);
    s32 idx = SCENE_M16(scene, pose_index) * 12;

    scene_record_bootstrap_task(task, (void *)(uintptr_t)(0x06074550 + idx));   /* braf tail call */
}

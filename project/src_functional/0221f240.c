/* scene_bootstrap_dispatch_52b0 [0x0221f240]: Existing map corroborated by source: record = 0x060752b0 + (s8)(scene->unk29*12) */
#include "dh_work.h"
#include "dh.h"
/* 0x0221f240  scene_bootstrap_dispatch_52b0 - tiny per-scene trampoline. Selects a
 * 12-byte record from the table at 0x060752b0 indexed by the current scene variant
 * (g_scene = *0x0609866c, scene->unk29) and tail-calls (braf) the shared bootstrap
 * state machine func_02218038(task, record).
 *
 * RE-SEGMENTED: the census member 0x0221f22e is pool-contaminated - its first 0x12
 * bytes are a trailing literal pool of the PREVIOUS function; the real dispatcher
 * code starts at 0x0221f240. find_entry.py skipped the whole member to 0x0221f2bc
 * (the next sibling), so this entry and its following handler (func_0221f262) had to
 * be segmented by hand. Split point: the instruction after the braf delay slot.
 *
 * fidelity: HIGH. braf target 0x218038 resolved (braf_resolve.py); table base
 * 0x060752b0 from the literal pool. Index math base + (s8)(scene->unk29 * 12).
 * Matches m2c_fn.py --asm. */

#define GP(a)  (*(void **)(uintptr_t)(a))

extern void scene_record_bootstrap_task(void *task, void *record);

void scene_bootstrap_dispatch_52b0(void *task) {
    void *scene = GP(DH_SCENE_DESCRIPTOR_ADDR);
    s32 idx = (s8)(SCENE_M8(scene, angle_variant) * 12);
    scene_record_bootstrap_task(task, (void *)(uintptr_t)(0x060752b0 + idx));   /* braf tail call */
}

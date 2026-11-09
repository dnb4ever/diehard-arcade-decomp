/* scene_bootstrap_dispatch_51c0 [0x0221eac0]: Existing map corroborated by source: record = 0x060751c0 + (u8)(scene->unk29*12) */
#include "dh_work.h"
#include "dh.h"
/* 0x0221eac0  scene_bootstrap_dispatch_51c0 - tiny per-scene trampoline. Selects a
 * 12-byte record from the table at 0x060751c0 indexed by the current scene variant
 * (g_scene = *0x0609866c, scene->unk29) and tail-calls (braf) the shared bootstrap
 * state machine func_02218038(task, record).
 *
 * RE-SEGMENTED: the census member 0x0221eaac is pool-contaminated (its first 0x14
 * bytes are a trailing literal pool of the previous function). find_entry.py maps it
 * to the real code entry 0x0221eac0, which is this dispatcher. The separate handler
 * past the braf (0x0221eae2) is split out as func_0221eae2.
 *
 * fidelity: HIGH. braf target 0x218038 resolved (braf_resolve.py); table base
 * 0x060751c0 from the literal pool. NOTE this member uses extu.b (unsigned) for the
 * index rather than exts.b, so the math is base + (u8)(scene->unk29 * 12). Matches
 * m2c_fn.py --asm. */

#define GP(a)  (*(void **)(uintptr_t)(a))

extern void scene_record_bootstrap_task(void *task, void *record);

void scene_bootstrap_dispatch_51c0(void *task) {
    void *scene = GP(DH_SCENE_DESCRIPTOR_ADDR);
    s32 idx = (u8)(SCENE_M8(scene, angle_variant) * 12);
    scene_record_bootstrap_task(task, (void *)(uintptr_t)(0x060751c0 + idx));   /* braf tail call */
}

/* scene_bootstrap_dispatch_4550 [0x0221b080]: Existing map corroborated by source: record = 0x06074550 + (s8)(scene->unk29*12) */
#include "dh_work.h"
#include "dh.h"
/* 0x0221b080  scene_bootstrap_dispatch_4550 - tiny per-scene trampoline. Selects a
 * 12-byte record from the table at 0x06074550 indexed by the current scene variant
 * (g_scene = *0x0609866c, scene->unk29) and tail-calls (braf) the shared bootstrap
 * state machine func_02218038(task, record).
 *
 * MERGED EXTENT: the separate handler past the braf (0x0221b0a2) is split out as
 * func_0221b0a2.
 *
 * fidelity: HIGH. braf target 0x218038 resolved (braf_resolve.py); table base
 * 0x06074550 from the literal pool. Index math base + (s8)(scene->unk29 * 12)
 * (asm *12 then exts.b). Matches m2c_fn.py --asm. */

#define GP(a)  (*(void **)(uintptr_t)(a))

extern void scene_record_bootstrap_task(void *task, void *record);

void scene_bootstrap_dispatch_4550(void *task) {
    void *scene = GP(DH_SCENE_DESCRIPTOR_ADDR);
    s32 idx = (s8)(SCENE_M8(scene, angle_variant) * 12);
    scene_record_bootstrap_task(task, (void *)(uintptr_t)(0x06074550 + idx));   /* braf tail call */
}

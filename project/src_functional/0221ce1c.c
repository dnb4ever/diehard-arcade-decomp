/* scene_bootstrap_dispatch_4b54 [0x0221ce1c]: Existing map corroborated by source: record = 0x06074b54 + (s8)(scene->unk29*12) */
#include "dh_work.h"
#include "dh.h"
/* 0x0221ce1c  scene_bootstrap_dispatch_4b54 - tiny per-scene trampoline. Selects a
 * 12-byte record from the table at 0x06074b54 indexed by the current scene variant
 * (g_scene = *0x0609866c, scene->unk29) and tail-calls (braf) the shared bootstrap
 * state machine func_02218038(task, record).
 *
 * This census "member" was a MERGED EXTENT: the extent walker swallowed the separate
 * handler at 0x0221ce3e past the unconditional braf. It is split out here; the
 * trailing handler is func_0221ce3e.
 *
 * fidelity: HIGH. braf target 0x218038 resolved (braf_resolve.py); record table base
 * 0x06074b54 and braf delta from the literal pool. The index math is base +
 * (s8)(scene->unk29 * 12) (asm: *12 then exts.b), i.e. record[scene->unk29] of a
 * 12-byte-stride array. Matches m2c_fn.py --asm. */

#define GP(a)  (*(void **)(uintptr_t)(a))

extern void scene_record_bootstrap_task(void *task, void *record);

void scene_bootstrap_dispatch_4b54(void *task) {
    void *scene = GP(DH_SCENE_DESCRIPTOR_ADDR);
    s32 idx = (s8)(SCENE_M8(scene, angle_variant) * 12);
    scene_record_bootstrap_task(task, (void *)(uintptr_t)(0x06074b54 + idx));   /* braf tail call */
}

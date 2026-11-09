#include "dh_work.h"
/* scene_bootstrap_record_select [0x02202dfa]: Selects the scene's twelve-byte bootstrap record, sets selection bit zero and delegates. */
#include "dh.h"
/* 0x02202dfa - tiny per-scene trampoline (scene_bootstrap_dispatch_4550 family).
 * Reads the current scene descriptor (scene = *0x0609866c), takes its signed
 * 16-bit state index at +0x28, selects the matching 12-byte record from the
 * table at 0x06074550 (record = base + state*12), sets bit0 of the
 * scene/selection flag word g_0609856c, and TAIL-CALLS (braf) the shared
 * record-driven bootstrap state machine func_02218038(task, record).
 *
 * MERGED EXTENT (important): fn.units/fn.extent report this entry as a ~1.3k-insn
 * "braf jump-table state machine" running past 0x20340c. That is an over-swallow
 * artifact: 0x202dfa is one of a chain of tiny braf-tail trampolines (siblings
 * 0x202d14, 0x202d7e, 0x202dda, 0x202dfa, ...) that share one literal pool and a
 * common braf target. The REAL func_02202dfa is only 0x202dfa..0x202e20 (18
 * insns); the words at 0x202e22..0x202e83 are DATA (the shared pool), and the
 * large FSM the prior pass feared is the SEPARATE sibling function at 0x202e84
 * (its own entry, a cmp/eq ladder on task->unk10). There is NO in-function braf
 * jump table here: the braf delta is the fixed constant 0x00015216
 * (0x202e1e + 4 + 0x15216 = 0x218038); only the DATA record pointer (r5) is
 * state-indexed - the per-state dispatch happens inside func_02218038. This is
 * the exact mirror of 0x0221b080 (same family), but indexed by a signed s16 at
 * scene+0x28 (mov.w, sign-extended) rather than (s8)scene->unk29, and with the
 * extra g_0609856c |= 1 side effect.
 *
 * fidelity: HIGH - hand-traced vs capstone / m2c_fn.py --asm. braf target
 * 0x218038 resolved (delta pool 0x202e80 = 0x00015216); table base 0x06074550
 * and globals 0x0609866c / 0x0609856c read from the literal pool. */

#define GP(a)   (*(void **)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern void scene_record_bootstrap_task(void *task, void *record);   /* scene_record_bootstrap_sm */

void scene_bootstrap_record_select(void *task) {
    void *scene = GP(DH_SCENE_DESCRIPTOR_ADDR);
    s32 idx = SCENE_M16(scene, pose_index) * 12;

    G32(0x0609856c) |= 1;
    scene_record_bootstrap_task(task, (void *)(uintptr_t)(0x06074550 + idx));   /* braf tail call */
}

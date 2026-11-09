#include "dh_work.h"
#include "dh.h"
/* 0x0220c782 (entity_phase_dispatch) - per-entity phase handler dispatch + post-update.
 *
 * ABI: handler(task). task = r4(arg0); entity = task->unk8 (r14 throughout).
 *
 * Runs one frame of an entity's state machine: when the entity has reached the
 * last phase slot (entity->unkA8 == g_phase_max-1) it re-selects the entity's
 * per-frame handler (entity->unk6C) from three RAM function-pointer tables, all
 * indexed by (entity->unkB2 + i) & 0xF:
 *   GUARD_TBL[idx](task)  - validator; first idx (i=0..2) returning nonzero wins
 *   NEXT_TBL[idx]         - handler installed into unk6C on a successful guard
 *   DEFAULT_TBL[idx]      - fallback handler installed when all 3 guards fail
 * The & 0xF table index IS the dispatch ("jump table") the fn is built around;
 * the tables are RAM-resident fn-ptr arrays populated elsewhere, rendered here as
 * the real indexed indirect load/call.
 *
 * Every frame (whether or not a new handler was selected) it then runs the
 * current handler, decays the camera shake, and rebuilds the entity matrices:
 *   handler(task); camera_shake_decay(task); mtx_build_fwd_inv(entity);
 * The original reserves a 32-byte on-stack scratch area across the handler call
 * and passes its pointer on the stack; the callee ABI for that slot is not
 * determinable from a RAM fn-ptr, so it is modelled here as handler(task).
 *
 * Bounded 0x20c782..0x20c858 (first rts/braf tail; fn.extent over-swallowed the
 * separate fn at 0x20c858). Tail calls braf-resolved to real TARGETs
 * (bsrf->0x0220d98e, braf->0x02214a7a); pool 0x114e/0x8222 are the PC-relative
 * displacements, not phantoms. fidelity: HIGH. */

typedef int  (*guard_fn)(void *task);
typedef void (*phase_fn)(void *task);

#define G_PHASE_MAX (*(s16 *)(uintptr_t)0x06052098) /* last valid phase index + 1 */
#define GUARD_TBL   ((guard_fn *)(uintptr_t)0x060520A4) /* [16] validators         */
#define NEXT_TBL    ((phase_fn *)(uintptr_t)0x060520E4) /* [16] next handlers      */
#define DEFAULT_TBL ((phase_fn *)(uintptr_t)0x06052124) /* [16] fallback handlers  */

extern void camera_shake_decay(void *task);   /* 0x0220d98e */
extern void mtx_build_fwd_inv(void *entity);  /* 0x02214a7a */

void entity_phase_dispatch(void *task)
{
    void *entity = TASK_PTR(task, work);
    phase_fn handler;

    if (M16(entity, 0xA8) == (s16)(G_PHASE_MAX - 1)) {
        s32 i = 0;
        for (;;) {
            s32 idx = (M16(entity, 0xB2) + i) & 0xF;
            if (GUARD_TBL[idx](task) != 0) {
                *(phase_fn *)((u8 *)entity + 0x6C) = NEXT_TBL[idx];
                break;
            }
            i += 1;
            if (i >= 3)
                break;
        }
        if (i >= 3)
            *(phase_fn *)((u8 *)entity + 0x6C) = DEFAULT_TBL[M16(entity, 0xB2) & 0xF];
    }

    handler = *(phase_fn *)((u8 *)entity + 0x6C);
    handler(task);
    camera_shake_decay(task);
    mtx_build_fwd_inv(entity);
}

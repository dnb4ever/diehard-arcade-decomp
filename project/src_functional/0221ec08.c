/* event_spawn_dispatch_draft [0x0221ec08]: Partial eleven-state event implementation retained as v1.09 SKIP. */
#include "dh_work.h"
#include "dh.h"
/* 0x0221ec08 evt_actor_spawn_dispatch - scripted-event actor step (task FSM).
 * arg0 = task (r4); task->unk8 = entity (r13); task->unk10 = state byte (r14+0x10).
 * ctx (r5) = *(*0x0609882C + 8); v (r4) = (s16)ctx->unkAE.
 * If (u32)(s8)state >= 11 -> return (cmp/hs after sign-extending mov.b).
 * Otherwise dispatch through the 11-entry word jump table at 0x21EC54
 * (mova base 0x21EC54, index state*2, braf).
 *
 * Resolved early-loaded registers (kept live across the dispatch):
 *   r10 = 0x21EC24 (mova) + 0xB9AC = 0x22A5D0  -> func_0222a5d0
 *   r11 = 0x0609856C  -> data record pointer passed alongside the entity
 *   r12 = 1           -> next state value stored to task->unk10
 * Case bodies (m2c-hinted): entity->unk30 = func_0221dfaa(v, ctx);
 * func_0222a5d0(entity, 0x0609856C); task->unk10 = 1.
 * fidelity: MEDIUM - dispatch guard/operands exact from asm; the per-state
 * bodies live past the braf in [0x21EC6C,0x21EDD2) whose target mapping comes
 * from the jump-table pool and the m2c draft (bsrf/braf targets unresolved). */

#define GP(a) (*(void **)(uintptr_t)(a))

extern s32 event_select_spawn_record_interior(s16 arg0, void *arg1);   /* 0x0221dfaa */
extern void process_spawn_list_entry(void *entity, void *rec); /* 0x0222a5d0 */

void event_spawn_dispatch_draft(void *task) {
    void *ctx = GP((uintptr_t)GP(DH_VIEW_TASK_ADDR) + 8);
    void *entity = TASK_PTR(task, work);
    s16 v = (s16)MU16(ctx, 0xAE);
    s32 state = (s8)TASK_S8(task, state);

    if ((u32)state >= 11) {
        return;
    }
    switch (state) {
    case 0:
        M32(entity, 0x30) = event_select_spawn_record_interior(v, ctx);
        process_spawn_list_entry(entity, (void *)(uintptr_t)0x0609856c);
        TASK_S8(task, state) = 1;
        break;
    default:
        /* remaining jump-table targets share the register setup above
         * (r10/r11/r12); their bodies are not in the reachable window */
        break;
    }
}

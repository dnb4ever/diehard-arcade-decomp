/* actor_commit_effect_handoffs [0x0223a9d0]: Clears two pending effect channels, hands their pose records to pooled effects and refreshes part offsets. */
#include "dh_work.h"
#include "dh.h"

/* 0x0223a9d0 - per-actor "retarget/handoff" update, tail-calling func_0223b254.
 *
 * MIRAGE CORRECTION: fn.py reports code 0x8ba (2234B) but that is again the
 * whole pool-unit [0x23a9b8,0x23b9b8).  The REAL func_0223a9d0 is only
 * [0x0223a9d0, 0x0223aada)  (~90 insn).  It ends with a delayed tail-jump
 * 'bra 0x23b254' (func_0223b254).  Of the 11 computed branches fn.py attributed
 * to this address, only ONE belongs here (func_022260c8 @0x23aa0a); the
 * 23e4a0/23cbb0/210020/20feec/... calls are in the siblings func_0223ab04 /
 * func_0223ab5c and later.
 *
 * arg0 = ent (r14), arg1 = a1 (r5 -> saved in r12, forwarded to func_0223a924).
 * Two near-identical halves (slots 0x6C/0x6E, then 0x6D/0x6F), each:
 *   - optionally dispatch func_0223a924(kind, a1, rec, ent)
 *   - clear the pending bytes, and copy a per-actor halfword param
 *     g_060E11E8/EA[ent->0x34] -> g_06096250/56[ent->0x34<<5].
 * rec base = 0x06097D58 + (ent->0x34 << 8), +0x30 / +0x50.
 *
 * m2c: no scaffold for this TU; hand-traced.  No tst #imm8 >= 0x80 occurs.
 * fidelity: HIGH. */

void actor_fill_pose_history(void *ent);                                  /* 0x022260c8 */
void effect_pool_update_pose(s32 kind, void *a1, void *rec, void *ent);     /* 0x0223a924 */
void actor_load_part_offset(void *ent);                                   /* 0x0223b254 */

void actor_commit_effect_handoffs(void *ent, void *a1)
{
    ACTOR_M32(ent, interaction_flags) &= 0xFF000000;

    /* g_06096C74[ent->0x34] = (u16)g_0607C3BC[ent->0x5E] */
    M32(0x06096C74, (ACTOR_WORK(ent)->slot_id << 2)) =
        (u16)M16(0x0607C3BC, (ACTOR_M16(ent, model_index) << 1));

    if (ACTOR_WORK(ent)->slot_id < 3) {
        actor_fill_pose_history(ent);
    }

    if (ACTOR_M8(ent, primary_effect_type) != 0) {
        void *rec = (void *)(uintptr_t)(0x06097D58
                        + ((s32)ACTOR_WORK(ent)->slot_id << 8) + 0x30);
        if ((s8)ACTOR_WORK(ent)->slot_id >= 3) {
            s32 k = (s8)ACTOR_M8(ent, primary_effect_type);
            if (k == 13 || k == 14) {
                effect_pool_update_pose(ACTOR_M8(ent, primary_effect_slot), a1, rec, ent);
            }
        } else {
            effect_pool_update_pose(ACTOR_M8(ent, primary_effect_slot), a1, rec, ent);
        }
        ACTOR_M8(ent, primary_effect_type) = 0;
        ACTOR_M8(ent, primary_effect_slot) = 0;
        M16(0x06096250, (ACTOR_WORK(ent)->slot_id << 5)) =
            (u16)M16(0x060E11E8, (ACTOR_WORK(ent)->slot_id << 2));
    }

    if (ACTOR_M8(ent, secondary_effect_type) != 0) {
        void *rec;
        if ((s8)ACTOR_M8(ent, secondary_effect_type) == 8) {
            M32(0x060E0F0C, (ACTOR_M8(ent, secondary_effect_slot) << 2)) = 7;
        }
        rec = (void *)(uintptr_t)(0x06097D58
                    + ((s32)ACTOR_WORK(ent)->slot_id << 8) + 0x50);
        effect_pool_update_pose(ACTOR_M8(ent, secondary_effect_slot), a1, rec, ent);
        ACTOR_M8(ent, secondary_effect_type) = 0;
        ACTOR_M8(ent, secondary_effect_slot) = 0;
        M16(0x06096256, (ACTOR_WORK(ent)->slot_id << 5)) =
            (u16)M16(0x060E11EA, (ACTOR_WORK(ent)->slot_id << 2));
    }

    actor_load_part_offset(ent);                    /* tail */
}

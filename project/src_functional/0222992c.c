#include "dh_work.h"
#include "dh.h"
/* 0x0222992c (actor_launch_dispatch) - per-actor launch/knockdown physics dispatcher.
 *
 * ABI (task, entity): task = r4(arg0), entity = r5(arg1, kept in r14). Part of the
 * 0x228xxx-0x229xxx actor state-step family; tail-calls animation_update (0x0222dc2c).
 *
 * PREAMBLE - one-shot hit reaction. Computes the per-zone record pointer
 *   zrec = g_06098734 + (s8)((entity->unk34 - 1) * 0x18)   (unk34 = actor slot).
 *   If (entity->unkCC == 0 && entity->unkB0 != 0): arm the reaction (unk4A=3, clear
 *   unkB0, set unkCC=1), fire the mode-specific reaction handler through the
 *   RAM-RESIDENT fn-ptr table g_060911BC indexed by the mode flag g_0609979A:
 *       (*g_060911BC[g_0609979A])(entity, 0xE);
 *   then clear unk4A. This is the shared pose/reaction table also used by
 *   actor_pose_dispatch (0x0222adaa); rendered faithfully as the documented
 *   indexed indirect call (NOT stubbed - the table lives in RAM, populated at
 *   runtime, so the index computation IS the dispatch).
 *
 * MAIN - switch on entity->unk88 (s32), a cmp/eq chain rendered as a C switch:
 *   case 0x54 (airborne/arc): integrate the launch arc. Horizontal nudge from the
 *     zone record flags (*zrec bit 0x100 -> unk78 += 0xE0, bit 0x200 -> unk78 -= 0xE0).
 *     Vertical: while unk58 < 8 accelerate (unkCE += 0x200; unkC4 += unkCE;
 *     unk7C = unkCE + unkC4); once unk58 >= 8 decelerate (unkC8 -= 0x400;
 *     unkC4 += unkC8; unk7C = unkC4 + unkC8). On landing (unk58 > 8 && unk7C < 0):
 *     zero unk78/7C/unkC, set unk88 = 0x56, run actor_facing_resolve(entity),
 *     set unk41 = 1 and the landing anim unk5C = g_06087A9A[entity->unk45] (8B stride).
 *   case 0x55 / 0x56 (settle): if this frame's loop flag unk54 & 2, reset the arc
 *     fields (unkB0/CE/CC/C8/C4 = 0), set unk5C = g_06096C74[unk34], clear unk48/unk88,
 *     then branch on the pose state: if (unk6D == 0 && (unk8C & 0x02000000))
 *       actor_pose_load(entity); actor_facing_resolve(entity); unk43 = 0x0D;
 *     else  unk84 = 1; unk52 = 0; unk43 = 1;
 *
 * TAIL - animation_update(task, entity)  (SH-2 braf tail-call to 0x0222dc2c).
 *
 * fidelity: HIGH - hand-traced vs braf_rewrite --asm (0x22992c..0x229af4, a braf
 * tail-call boundary; the sibling 0x229af4 actor_launch_countdown is in 0229af4.c).
 * bsrf resolved: actor_facing_resolve (0x02227704), actor_pose_load (0x0223a6dc),
 * animation_update (0x0222dc2c). SH-2 immediates were decimal; offsets per m2c. */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define TABP(a)  (*(void **)(uintptr_t)(a))   /* RAM fn-ptr table entry */

extern s32  animation_update(void *task, void *entity);  /* 0x0222dc2c */
extern void actor_facing_resolve(void *entity);          /* 0x02227704 */
extern void actor_pose_load(void *entity);               /* 0x0223a6dc */

void actor_launch_dispatch(void *task, void *entity) {
    /* per-zone record for this actor slot (g_06098734, 0x18-byte stride) */
    s16 *zrec = (s16 *)(uintptr_t)(DH_PLAYER_INPUT_RECORDS_ADDR + (s8)((MU8(entity, ACTOR_SLOT_ID) - 1) * 0x18));

    if (MU16(entity, 0xCC) == 0 && M32(entity, 0xB0) != 0) {
        ACTOR_M8(entity, hit_source_slot) = 3;
        M32(entity, 0xB0) = 0;
        MU16(entity, 0xCC) = 1;
        ((void (*)(void *, s32))TABP(DH_ACTOR_POSE_HANDLERS_ADDR + RU16A(DH_ACTOR_POSE_SET_ADDR) * 4))(entity, 0xE);
        ACTOR_M8(entity, hit_source_slot) = 0;
    }

    switch (ACTOR_WORK(entity)->action_state) {
    case 0x54: {
        s16 flags = *zrec;
        if ((u16)flags & 0x100)
            M32(entity, 0x78) += 0xE0;
        else if ((u16)flags & 0x200)
            M32(entity, 0x78) -= 0xE0;

        if ((s32)ACTOR_U16(entity, animation_frame) < 8) {
            M16(entity, 0xCE) += 0x200;
            M32(entity, 0xC4) += M16(entity, 0xCE);
            M32(entity, 0x7C)  = M16(entity, 0xCE) + M32(entity, 0xC4);
        } else {
            M32(entity, 0xC8) -= 0x400;
            M32(entity, 0xC4) += M32(entity, 0xC8);
            M32(entity, 0x7C)  = M32(entity, 0xC4) + M32(entity, 0xC8);
        }

        if ((s32)ACTOR_U16(entity, animation_frame) > 8 && M32(entity, 0x7C) < 0) {
            M32(entity, 0x78) = 0;
            M32(entity, 0x7C) = 0;
            ACTOR_WORK(entity)->position_y  = 0;
            ACTOR_WORK(entity)->action_state = 0x56;
            actor_facing_resolve(entity);
            ACTOR_WORK(entity)->animation_control = DH_ANIMATION_CONTROL_RESTART;
            ACTOR_WORK(entity)->animation_requested = *(s16 *)(uintptr_t)(0x06087A9A + MU8(entity, 0x45) * 8);
        }
        break;
    }
    case 0x55:
    case 0x56:
        if (ACTOR_U16(entity, animation_events) & 2) {
            M32(entity, 0xB0) = 0;
            M16(entity, 0xCE) = 0;
            M16(entity, 0xCC) = 0;
            M32(entity, 0xC8) = 0;
            M32(entity, 0xC4) = 0;
            ACTOR_WORK(entity)->animation_requested = (s16)R32A(0x06096C74 + MU8(entity, ACTOR_SLOT_ID) * 4);
            ACTOR_M8(entity, action_pending)  = 0;
            ACTOR_WORK(entity)->action_state = 0;
            if (ACTOR_MU8(entity, secondary_effect_type) == 0 && (ACTOR_M32(entity, interaction_flags) & 0x02000000)) {
                actor_pose_load(entity);
                actor_facing_resolve(entity);
                ACTOR_WORK(entity)->state_handler = 0x0D;
            } else {
                ACTOR_WORK(entity)->action_category = 1;
                ACTOR_M16(entity, interaction_latch) = 0;
                ACTOR_WORK(entity)->state_handler  = DH_PLAYER_HANDLER_IDLE;
            }
        }
        break;
    }

    animation_update(task, entity);   /* SH-2 braf tail-call */
}

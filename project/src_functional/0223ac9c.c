/* actor_spawn_special_debris [0x0223ac9c]: Spawns variant-specific debris, clears its request and refreshes slot render offsets. */
#include "dh.h"
/* 0x0223ac9c (actor_state_gen_special) - special-move state generator. For a nonzero
 * entity->unk6D (signed byte) it spawns an effect/debris task and seeds it, then clears
 * the special request; finally, for player slots (unk34 < 3) it refreshes HUD/record
 * state and tail-calls func_0223b254.
 *
 * Table select (same idiom as func_0223ac50): if entity->unkE4 == 6 the table is
 *   g_0608CE64[(unkE0 & 0x01000000) ? 1 : 0]; else g_0608CE04[(s8)unk6D]; 8-byte stride.
 * Only the default case reads the selected record (word @+2 -> spawned->unk60).
 *
 * The dispatch is a cmp/eq #4 / #10 / #11 chain on (s8)unk6D (NOT a jump table) with a
 * default. Every case spawns via func_0220ff28(g_06099580, handler)->unk8:
 *   case 4  -> effect_tumble_task (func_0223d2a0)
 *   case 10 -> func_0223d3d0
 *   case 11 -> func_0223d558
 *   default -> effect_tumble_task (func_0223d2a0)  [default shares case 4's handler;
 *              the 0x0223d1b6/effect_fall_task pointer belongs to the following function,
 *              a m2c over-swallow artifact - confirmed via ptr_resolve on the bounded range].
 *
 * fidelity: HIGH - widths from the loads (unk6D/44/34/6A/5E-index bytes & words, unk8C/84
 *   longs; spawned unk40/48/70/64/68/08-10 long stores, unk60/30/62 word stores, unk34 byte);
 *   negative immediates are .short pool values sign-extended by mov.w (e.g. 0xFC00=-0x400);
 *   unk60 is a 16-bit store; default's unk60 = record word @+2. Spawn-arg ctx is *0x06099580.
 *   Calls/pointers resolved via decomp_bounded + ptr_resolve over end 0x23af0c; func_022260c8
 *   and func_0223b254 each take one arg (the trailing r5 in raw m2c is a dead register). */

extern void *task_spawn_list_work7c(s32 ctx, void *handler);  /* spawn_from_template -> holder */
extern void actor_fill_pose_history(void *entity);             /* per-id record fill */
extern void actor_load_part_offset(void *entity);             /* copy action record (tail) */
extern s32  g_06099580;                              /* spawn pool/list context */

void actor_spawn_special_debris(void *entity) {
    s8 sel = M8(entity, 0x6D);

    if (sel != 0) {
        void *tbl;
        void *obj;

        if (M32(entity, 0xE4) == 6) {
            s32 idx = (M32(entity, 0xE0) & 0x01000000) ? 1 : 0;
            tbl = (u8 *)((uintptr_t)0x0608CE64 + idx * 8);
        } else {
            tbl = (u8 *)((uintptr_t)0x0608CE04 + sel * 8);
        }

        /* PIC handler addresses below are the relocated HWRAM values. */
        switch (sel) {
        case 4:
            obj = MP(task_spawn_list_work7c(g_06099580, (void *)(uintptr_t)0x060412a0), 8);
            M32(obj, 0x40) = M8(entity, 0x44) ? -0x400 : 0x400;
            M32(obj, 0x48) = M8(entity, 0x44) ? 0x1800 : -0x1800;
            M32(obj, 0x70) = 0x2800;
            M16(obj, 0x60) = 0x10;
            M32(obj, 0x64) = 0x8000;
            break;
        case 10:
            obj = MP(task_spawn_list_work7c(g_06099580, (void *)(uintptr_t)0x060413d0), 8);
            M32(obj, 0x40) = M8(entity, 0x44) ? -0x4000 : 0x4000;
            M32(obj, 0x70) = 0x1600;
            M16(obj, 0x60) = 0x14;
            M32(obj, 0x64) = 0x8000;
            break;
        case 11:
            obj = MP(task_spawn_list_work7c(g_06099580, (void *)(uintptr_t)0x06041558), 8);
            M32(obj, 0x40) = M8(entity, 0x44) ? -0x3800 : 0x3800;
            M32(obj, 0x70) = 0x1600;
            M16(obj, 0x60) = 0x0F;
            M32(obj, 0x64) = 0x8000;
            break;
        default:
            obj = MP(task_spawn_list_work7c(g_06099580, (void *)(uintptr_t)0x060412a0), 8);
            M32(obj, 0x40) = M8(entity, 0x44) ? -0x4000 : 0x4000;
            M32(obj, 0x70) = 0x1000;
            M16(obj, 0x60) = M16(tbl, 2);
            M32(obj, 0x64) = 0x1999;
            break;
        }

        /* shared post-switch seed block */
        {
            void *slot = (u8 *)((uintptr_t)0x06097D88 + (M8(entity, 0x34) << 8));
            M32(obj, 0x08) = M32(slot, 0);
            M32(obj, 0x0C) = M32(slot, 4);
            M32(obj, 0x10) = M32(slot, 8);
        }
        M16(obj, 0x30) = M16((u8 *)((uintptr_t)0x060E1080 + M8(entity, 0x6D) * 2), 0);
        M32(obj, 0x68) = M32((u8 *)((uintptr_t)DH_ACTOR_SLOT_MASKS_ADDR + (M8(entity, 0x34) << 2)), 0);
        M8(obj, 0x34)  = M8(entity, 0x34);
        M16(obj, 0x62) = 0x41;

        M16((u8 *)((uintptr_t)0x06096256 + (M8(entity, 0x34) << 5)), 0) =
            M16((u8 *)((uintptr_t)0x060E11EA + (M8(entity, 0x34) << 2)), 0);
        M32((u8 *)((uintptr_t)0x060E0F2C + (M8(entity, 0x6F) << 2)), 0) = 5;
        M32(entity, 0x8C) &= ~M32((u8 *)((uintptr_t)DH_ACTOR_SLOT_MASKS_ADDR + (M8(entity, 0x6D) << 2)), 0);

        M8(entity, 0x6D) = 0;
        M8(entity, 0x6F) = 0;
        M8(entity, 0x45) = 0;
    }

    if (M8(entity, 0x34) < 3) {
        if (M32(entity, 0x84) & 0x00400000) {
            M8(entity, 0x43) = M8(entity, 0x6A) ? 0x29 : 0x22;
        }
        M32((u8 *)((uintptr_t)0x06096C74 + (M8(entity, 0x34) << 2)), 0) =
            MU16((u8 *)((uintptr_t)0x0607C3BC + (M16(entity, 0x5E) * 2)), 0);
        actor_fill_pose_history(entity);
    }

    actor_load_part_offset(entity);
}

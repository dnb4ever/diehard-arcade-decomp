#include "dh_work.h"
/* enemy_resolve_team_pairs [0x02243628]: Builds active enemy lists, sorts group metrics and assigns approach pairs and leaders. */
#include "dh.h"

/* 0x02243628 - per-frame team / pairing resolver (void)(void).
 *
 * Operates on a fixed set of HWRAM tables. Five phases:
 *   (1) build an "active" list @0x060970A8 (32-byte entries) from slots 3..6,
 *       dispatching on *(0x06098564) 0..3 -> func_02243be0;
 *   (2) cleanup pass -> func_02243e22 for entries matching a flag test;
 *   (3) group members by *(0x060E0F0C)[0..8] into 0x06096EC4 buckets, run the
 *       func_0223e530 sort/assign, then stamp group leaders;
 *   (4) build pair buckets (metric = sum of two squared coords) in the same
 *       0x06096EC4 scratch;
 *   (5) func_0223e530 again, then a 4-wide assign loop -> func_02243d78/func_02243db4.
 *
 * m2c structured phases 1,2,4(control),5 cleanly but emitted M2C_ERROR at all
 * four computed calls (bsrf). Those were resolved by hand from the disassembly
 * + braf_resolve:
 *   0x24388c, 0x2439f0 -> func_0223e530   (sort; takes NO register args)
 *   0x24397e, 0x243996 -> func_022131c8  (phase-4 pairing)
 * braf_resolve CORRECTION (pool-phantom trap): "0x243814 bsrf r0" is NOT a
 * branch - .Lpool_243814 = 0x0003FFFF, a mask literal loaded with mov.l and
 * used at .L2437d2 (tst 0x3FFFF, obj->0x8C). The function has only 4 real bsrf.
 *
 * func_022131c8 RESOLVED (its prologue is `dmuls.l r4,r5 / sts mach,r1 /
 * sts macl,r0 / rts / xtrct r1,r0`): a 2-arg 16.16 fixed-point multiply that
 * returns the middle 32 bits of the 64-bit product, i.e. ((s64)a*(s64)b) >> 16.
 * At BOTH phase-4 call sites the delay slot (`mov r5,r4`) leaves r4 == r5 == the
 * just-loaded coord, so each call squares one coord. The two results are then
 * ADDED (asm `add r1,r4`), confirming the summation sign the note flagged:
 * metric = sq(coordA) + sq(coordB)  (a sum of squares, not a difference).
 *
 * Tables (HWRAM): slotActive 0x06099590 s32[], slotPtrs 0x060995B0 void*[]
 * (deref +8 -> inner object); pairMark u8[4] @0x06096EC0; active list @0x060970A8
 * (stride 32: +0 obj ptr, +4 used-flag); bucket scratch @0x06096EC4 (stride 44:
 * +0, +4 group/pair id, +8 count, entries from +12 each 8 bytes); bucket count
 * @0x06097050; sort result array @0x06097070 (stride 8: +0 status, +4 index).
 *
 * fidelity: HIGH on control flow / memory effects / calls. Return type void
 * (every exit is rts with no return value). func_0223e530 modelled as (void) since
 * neither call site sets any argument register.
 */

s32  enemy_commit_target_approach(void *obj, s32 which, void *list);     /* extern */
void enemy_setup_lateral_approach(void *obj);                            /* extern */
void enemy_assign_minimum_cost_requests(void);                                 /* extern (sort) */
s32  fixed_mul16(s32 a, s32 b);                         /* extern ((a*b)>>16) */
void enemy_init_jump_lane(void *a, void *b, s32 pairIdx, s32 flag); /* extern */
void enemy_commit_dodge_lane(void *a, void *b, s32 pairIdx, s32 flag); /* extern */

void enemy_resolve_team_pairs(void)
{
    s32   *slotActive  = (s32  *)(uintptr_t)DH_ACTOR_SLOT_IDS_ADDR;
    void **slotPtrs    = (void **)(uintptr_t)DH_ACTOR_SLOT_TASKS_ADDR;
    u8    *pairMark    = (u8   *)(uintptr_t)0x06096EC0;
    s32   *bucketCount = (s32  *)(uintptr_t)0x06097050;
    s32    active_n = 0;
    s8    *entry;
    s32    slot;

    /* --- phase 0: reset bookkeeping --- */
    M32(pairMark, 0) = 0;
    M32((void *)(uintptr_t)0x06097098, 0x00) = 0;
    M32((void *)(uintptr_t)0x06097098, 0x04) = 0;
    M32((void *)(uintptr_t)0x06097098, 0x08) = 0;
    M32((void *)(uintptr_t)0x06097098, 0x0C) = 0;
    M32((void *)(uintptr_t)0x060970A8, 0x00) = 0;

    /* --- phase 1: build active list from slots 3..6 --- */
    entry = (s8 *)(uintptr_t)0x060970A8;
    for (slot = 3; slot <= 6; slot++) {
        void *obj;

        if (slotActive[slot] == 0)
            continue;
        obj = MP(slotPtrs[slot], 8);
        ACTOR_M32(obj, state_flags) &= ~0x300;
        switch (M32((void *)(uintptr_t)DH_READY_PLAYER_MASK_ADDR, 0)) {
        case 0:
            ACTOR_M32(obj, state_flags) |= 0x04000000;
            M32(pairMark, 0) = -1;
            break;
        case 1:
            enemy_commit_target_approach(obj, 1, entry);
            pairMark[3] = 0x71;
            pairMark[1] = 0x71;
            break;
        case 2:
            enemy_commit_target_approach(obj, 2, entry);
            pairMark[2] = 0x71;
            pairMark[0] = 0x71;
            break;
        case 3:
            if (enemy_commit_target_approach(obj, 1, entry) == 0)
                enemy_commit_target_approach(obj, 2, entry);
            break;
        }
        if ((ACTOR_M32(obj, state_flags) & 0x04000100) == 0) {
            active_n++;
            MP(entry, 0) = obj;
            M32(entry, 4) = 0;
            entry += 32;
        }
    }
    if (active_n == 0)
        return;

    /* --- phase 2: cleanup pass --- */
    entry = (s8 *)(uintptr_t)0x060970A8;
    for (slot = 0; slot < active_n; slot++, entry += 32) {
        void *obj;

        if (M32(entry, 4) != 0)
            continue;
        obj = MP(entry, 0);
        if ((ACTOR_M32(obj, interaction_flags) & 0x1E) != 0 || M8(obj, 0x46) == 1) {
            enemy_setup_lateral_approach(obj);
            M32(entry, 4) = 1;
        }
    }

    /* --- phase 3: group members by group table --- */
    if (M32((void *)(uintptr_t)0x060E0E88, 0) != 0) {
        s8  *bucket = (s8 *)(uintptr_t)0x06096EC4;
        s32  grp;

        *bucketCount = 0;
        for (grp = 0; grp < 9; grp++) {
            s32 gid = M32((void *)(uintptr_t)0x060E0F0C, grp * 4);
            s32 mask;
            s32 j;

            if (!(gid != 0 && gid < 7 &&
                  M32((void *)(uintptr_t)0x060E0F2C, grp * 4) == 1))
                continue;

            M32(bucket, 8) = 0;
            mask = M32((void *)(uintptr_t)DH_ACTOR_SLOT_MASKS_ADDR, gid * 4);
            entry = (s8 *)(uintptr_t)0x060970A8;
            for (j = 0; j < active_n; j++, entry += 32) {
                void *obj;
                s32   metric;
                s32   cnt;

                if (M32(entry, 4) != 0)
                    continue;
                obj = MP(entry, 0);
                if (ACTOR_M32(obj, interaction_flags) & 0x3FFFF)
                    continue;
                if (!(M32(obj, 0xFC) & mask))
                    continue;
                metric = M32((void *)(uintptr_t)0x060E0F6C,
                             (ACTOR_M8(obj, slot_id) << 5) + grp * 4);
                if ((u32)metric >= 0x100000U)
                    continue;
                cnt = M32(bucket, 8);
                M32(bucket, 12 + cnt * 8 + 0) = ACTOR_M8(obj, slot_id);
                M32(bucket, 12 + cnt * 8 + 4) = metric;
                M32(bucket, 8) = cnt + 1;
            }
            if (M32(bucket, 8) != 0) {
                (*bucketCount)++;
                M32(bucket, 0) = 0;
                M32(bucket, 4) = grp;
                bucket += 44;
            }
        }

        if (*bucketCount != 0) {
            s8 *pr = (s8 *)(uintptr_t)0x06097070;
            s32 s;

            enemy_assign_minimum_cost_requests();
            for (s = 3; s < 7; s++, pr += 8) {
                void *objA;
                s32   k;

                if (M32(pr, 0) != 0)
                    continue;
                objA = MP(slotPtrs[s], 8);
                M32(objA, 0x12C) =
                    M8(MP(MP((void *)(uintptr_t)0x060E0F4C, M32(pr, 4) * 4), 8),
                       0x34);
                M32(objA, 0x130) = 0;
                M32(objA, 0x134) = 0;
                M32(objA, 0xE0) &= ~0x30;
                M32(objA, 0xE0) |= 0x40;
                entry = (s8 *)(uintptr_t)0x060970A8;
                for (k = 0; k < active_n; k++, entry += 32) {
                    if (M8(MP(entry, 0), 0x34) == s)
                        M32(entry, 4) = 1;
                }
            }
        }
    }

    /* --- phase 4: build pair buckets (metric = sum of squared coords) --- */
    *bucketCount = 0;
    {
        s8 *bucket = (s8 *)(uintptr_t)0x06096EC4;
        s32 pairIdx;

        for (pairIdx = 0; pairIdx < 4; pairIdx++) {
            s32 b0, b1;
            s32 j;

            if (pairMark[pairIdx] == 0x71)
                continue;

            M32(bucket, 8) = 0;
            b0 = pairIdx & 1;
            b1 = (pairIdx >> 1) & 1;
            entry = (s8 *)(uintptr_t)0x060970A8;
            for (j = 0; j < active_n; j++, entry += 32) {
                void *obj;
                s32   v1, v2;
                s32   cnt;

                if (M32(entry, 4) != 0)
                    continue;
                obj = MP(entry, 0);
                cnt = M32(bucket, 8);
                M32(bucket, 12 + cnt * 8 + 0) = ACTOR_M8(obj, slot_id);

                v1 = M32(entry, 8 + b0 * 8 + b1 * 4);
                v2 = M32(entry, 24 + b0 * 4);
                M32(bucket, 12 + cnt * 8 + 4) =
                    fixed_mul16(v1, v1) + fixed_mul16(v2, v2);

                M32(bucket, 8) = cnt + 1;
            }
            if (M32(bucket, 8) != 0) {
                (*bucketCount)++;
                M32(bucket, 0) = 0;
                M32(bucket, 4) = pairIdx;
                bucket += 44;
            }
        }
    }
    if (*bucketCount == 0)
        return;

    /* --- phase 5: assign pairs --- */
    enemy_assign_minimum_cost_requests();
    {
        s8 *pr = (s8 *)(uintptr_t)0x06097070;
        s32 slotIdx;

        for (slotIdx = 3; slotIdx < 7; slotIdx++, pr += 8) {
            void *objA, *objB;
            s32   pairIdx;
            s32   flag;

            if (M32(pr, 0) < 0)
                continue;
            pairIdx = M32(pr, 4);
            objA = MP(slotPtrs[slotIdx], 8);
            M32(objA, 0x128) = (pairIdx & 1) + 1;
            objB = MP(slotPtrs[M32(objA, 0x128)], 8);
            flag = (M32(objB, 0x84) & 0x20000) ? 1 : 0;
            if (pairMark[pairIdx] == 0 && M32(pr, 0) == 0 &&
                !(M32(objB, 0x84) & 0x40000) && flag == 0)
                enemy_init_jump_lane(objA, objB, pairIdx, flag);
            else
                enemy_commit_dodge_lane(objA, objB, pairIdx, flag);
        }
    }
}

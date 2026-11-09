#include "dh_work.h"
#include "dh.h"
/* 0x02226eb8 (actor_clash_state) - per-frame handler for a two-actor "clash" /
 * tie-up interaction (e.g. attacks or grabs colliding), run as an 11-state
 * machine. task = arg0; pair = task->unk8; the two combatants are A = pair->16
 * (r14) and B = pair->20 (r13), both held as persistent cursors for the whole
 * body. A frame counter A->unk50 is bumped at entry.
 *
 * PREAMBLE builds, for each actor, a "flag" (actor->unk34 < 3) and a combo
 * descriptor pointer desc = 0x06098734 + (s8)((unk34-1)*24). When an actor's
 * descriptor slot is free (B<3 path) or a priority gate passes (B>=3 path) and
 * the global mode 0x06098670 == 1 with the clash counter 0x06098830 already >=7,
 * it latches a "hit" record (clamps 0x06098830 to 7, sets flag byte +13 of
 * *0x0609882c, and writes both unk34's + 0 into the record object
 * *(*0x0609882c)->unk8 at +0xAE/0xB0/0xB2). Then, if A->unk48 != 0 and
 * A->unk4A != B->unk34, it runs the PIC handler (table 0x060911bc indexed by
 * 0x0609979a) and forces state 10.
 *
 * DISPATCH: state = MU8(task,0x10); guard >= 11 returns; 11-way braf jump table
 * at 0x227088 (base 0x227076) decoded directly from ROM:
 *   0->0x22709e 1->0x2270c4 2->0x2271d0 3->0x2272f8 4->0x22736a 5->0x2273a4
 *   6/7->0x227444 8->0x22748e 9->0x2274d2 10->0x22759c.
 *   0 : init, zero A->unk50 / B->unkC4 / B->unkCC, copy A->unk34 to B->unk4A,
 *       advance; FALLS THROUGH to state 1.
 *   1 : run anim for both; if either actor is in hitstun (unk54&2) -> state 5;
 *       else, per flag/descriptor, arm the pair (unk41) and seed unk5C codes,
 *       optionally -> state 2; then on B's descriptor advance B->unkC4 and, past
 *       5, -> state 5.
 *   2 : if A valid run func_02227602(descA,A,B) and maybe func_022275e4 -> state
 *       4; else if actor_script_step(A)==1 flag a move entry. Then anim; fire the PIC
 *       on A->unk66; a 0x06096ca0 marker of 3 -> state 3; else on B advance
 *       unkC4 past 8 -> state 5.
 *   3 : anim; on hitstun snap unk84/unk88, func_0222ad7a(B) -> state 10; else
 *       fire PIC on A->unk66.
 *   4 : anim; on hitstun -> state 5.
 *   5 : pick a follow-up index (rng / descriptor), seed unk5C from table
 *       0x060862fc, arm the pair, -> state index+6 (6..9); anim; clamp B->unk56.
 *   6/7: anim; on hitstun set unk88=69, func_0222ad7a(A) -> state 10.
 *   8 : anim; on hitstun set unk88=70 -> state 10.
 *   9 : anim; on hitstun snap A to B (unk8/unk16, unk88=71, clear flags),
 *       reset task->unk0; else ease A's unk8/unk16 halfway toward B.
 *   10: teardown - clear the 0x20000000 bit, publish unk4B/unk4C into unk100,
 *       clear flags, reset task->unk0.
 *
 * ABI (task r4). fidelity: HIGH (structure) - 11-way braf table decoded from ROM
 * (m2c rendered it as linear fallthrough and mangled the table bytes). Every
 * handler hand-traced vs m2c_fn.py --asm; callee-saved r8-r14 pinned (r8 =
 * animation_update 0x22dc2c via mova+add; r14/r13 = A/B; r9=1, r12=0). Calls:
 * bsr siblings func_022275e4 (0x2275e4, 1-arg) and func_02227602 (0x227602,
 * 3-arg); bsrf actor_script_step (0x022449b2, returns), func_0222ad7a (0x22ad7a),
 * rng_next (0x210020); all via braf_resolve. The PIC table 0x060911bc[0x0609979a]
 * call appears 3x. unk5C/unk50/unk54/unk26/unk56/unkCC are halfwords (mov.w).
 * NAMING of the routine is a GUESS. */

extern s32  actor_script_step(u8 *actor);                    /* 0x022449b2 */
extern void actor_decode_pose_direction(void *a);                         /* 0x22ad7a */
extern void actor_combo_reset(void *a);                         /* 0x2275e4 sibling */
extern void interaction_advance_frame_pair(void *desc, void *a, void *b);     /* 0x227602 sibling */
extern u32  rng_next(void);                                 /* 0x210020 */
extern void animation_update(void *desc, void *actor);      /* 0x22dc2c (r8) */

/* run the current animation descriptor for both combatants (jsr @r8 pair) */
static void anim_pair(void *a, void *b) {
    animation_update(MP(DH_ACTOR_SLOT_TASKS_ADDR, M8(a, 0x34) * 4), a);
    animation_update(MP(DH_ACTOR_SLOT_TASKS_ADDR, M8(b, 0x34) * 4), b);
}

/* PIC handler dispatched through table 0x060911bc by mode index 0x0609979a */
static void pic_dispatch(void *actor, s32 arg) {
    void (*fn)(void *, s32) =
        (void (*)(void *, s32))MP(DH_ACTOR_POSE_HANDLERS_ADDR, MU16(DH_ACTOR_POSE_SET_ADDR, 0) * 4);
    fn(actor, arg);
}

/* latch a clash "hit" record (shared by both preamble priority paths) */
static void register_clash_hit(void *a, void *b) {
    void *rec = MP(MP(DH_VIEW_TASK_ADDR, 0), 8);
    M32(DH_VIEW_REQUEST_PRIORITY_ADDR, 0) = 7;
    M8(MP(DH_VIEW_TASK_ADDR, 0), 13) = 1;
    M16(rec, 0xAE) = M8(a, 0x34);
    M16(rec, 0xB0) = M8(b, 0x34);
    M16(rec, 0xB2) = 0;
}

void actor_clash_state(void *task) {
    void *pair = TASK_PTR(task, work);
    void *A = MP(pair, 16);
    void *B = MP(pair, 20);
    s32 mode = M32(DH_PLAY_SESSION_MODE_ADDR, 0);
    s32 flagA = (M8(A, 0x34) < 3);
    s32 flagB = (M8(B, 0x34) < 3);
    void *descA = (void *)(uintptr_t)(DH_PLAYER_INPUT_RECORDS_ADDR + (s8)((M8(A, 0x34) - 1) * 24));
    void *descB = (void *)(uintptr_t)(DH_PLAYER_INPUT_RECORDS_ADDR + (s8)((M8(B, 0x34) - 1) * 24));

    M16(A, 0x50) += 1;

    if (flagB) {
        s32 slotIdx = (M8(B, 0x34) == 1) ? 2 : 1;
        if (M32(DH_ACTOR_SLOT_IDS_ADDR + slotIdx * 4, 0) == 0) {
            if (mode == 1 && M32(DH_VIEW_REQUEST_PRIORITY_ADDR, 0) >= 7)
                register_clash_hit(A, B);
            M32(A, 0x84) |= 0x20000000;
        }
    } else {
        if (M32(0x0609958c, 0) == 1 && mode == 1 && M32(DH_VIEW_REQUEST_PRIORITY_ADDR, 0) >= 7)
            register_clash_hit(A, B);
    }

    if (M8(A, 0x48) != 0 && M8(A, 0x4A) != M8(B, 0x34)) {
        pic_dispatch(A, M16(A, 0x64));
        M32(B, 0x88) = 73;
        M32(A, 0x88) = 73;
        M8(B, 0x4A) = M8(A, 0x4A);
        M16(B, 0x60) = M16(A, 0x60);
        M16(B, 0x62) = M16(A, 0x62);
        TASK_S8(task, state) = 10;
    }

    if (TASK_U8(task, state) >= 11)
        return;

    switch (TASK_U8(task, state)) {
    case 0:
        if (flagA)
            actor_combo_reset(A);
        M16(A, 0x50) = 0;
        M32(B, 0xC4) = 0;
        M16(B, 0xCC) = 0;
        M8(B, 0x4A) = M8(A, 0x34);
        TASK_S8(task, state) += 1;
        /* fallthrough */
    case 1:
        anim_pair(A, B);
        if ((MU16(A, 0x54) & 2) || (MU16(B, 0x54) & 2)) {
            M32(B, 0xC4) = 0;
            M8(B, 0x41) = 1;
            M8(A, 0x41) = 1;
            TASK_S8(task, state) = 5;
            return;
        }
        if (flagA) {
            if ((u16)M16(descA, 4) & 3) {
                TASK_S8(task, state) = 2;
                M8(B, 0x41) = 1;
                M8(A, 0x41) = 1;
                if ((u16)M16(descA, 4) & 1) {
                    M16(0x06096ea0 + M8(A, 0x34) * 2, 0) = 0;
                    M16(A, 0x5C) = 0xC2;
                    M16(B, 0x5C) = 0xC3;
                } else {
                    M16(0x06096ea0 + M8(A, 0x34) * 2, 0) = 1;
                    M16(A, 0x5C) = 0xCA;
                    M16(B, 0x5C) = 0xCB;
                }
            }
        } else if (actor_script_step(A) == 1) {
            TASK_S8(task, state) = 2;
            M32(A, 0xC4) = 0;
            M32(B, 0xC4) = 0;
            M8(B, 0x41) = 1;
            M8(A, 0x41) = 1;
            M16(B, 0x5C) = M16(A, 0x5C) + 1;
        }
        if (!flagB)
            return;
        if (M16(descB, 4) != 0) {
            M32(B, 0xC4) += 1;
            M16(B, 0xCC) = M16(descB, 4) & 7;
        }
        if (M32(B, 0xC4) > 5) {
            M8(B, 0x41) = 1;
            M8(A, 0x41) = 1;
            TASK_S8(task, state) = 5;
        }
        return;
    case 2:
        if (flagA) {
            interaction_advance_frame_pair(descA, A, B);
            if (M16(0x06096e90 + M8(A, 0x34) * 2, 0) != 0) {
                actor_combo_reset(A);
                TASK_S8(task, state) = 4;
            }
        } else if (actor_script_step(A) == 1) {
            void *mv = (void *)((u8 *)MP(A, 0x118) + (u16)M16(A, 0x11C) * 4);
            if (MU16(mv, 2) & 0x8000)
                M16(0x06096ca0 + M8(A, 0x34) * 2, 0) = 3;
            if (M8(A, 0x41) == 1) {
                M8(B, 0x41) = 1;
                M16(B, 0x5C) = M16(A, 0x5C) + 1;
            }
        }
        anim_pair(A, B);
        if (M16(A, 0x66) != 0) {
            pic_dispatch(B, M16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        if (M16(0x06096ca0 + M8(A, 0x34) * 2, 0) == 3) {
            actor_combo_reset(A);
            TASK_S8(task, state) = 3;
            return;
        }
        if (!flagB)
            return;
        if (M16(descB, 4) != 0) {
            M32(B, 0xC4) += 1;
            M16(B, 0xCC) = M16(descB, 4) & 7;
        }
        if (M32(B, 0xC4) > 8) {
            actor_combo_reset(A);
            M8(B, 0x41) = 1;
            M8(A, 0x41) = 1;
            TASK_S8(task, state) = 5;
        }
        return;
    case 3:
        anim_pair(A, B);
        if (MU16(A, 0x54) & 2) {
            M32(B, 0x84) = 0x1000;
            M32(B, 0x88) = 72;
            M32(A, 0x88) = 72;
            M16(A, 0x52) = 0;
            actor_decode_pose_direction(B);
            TASK_S8(task, state) = 10;
            return;
        }
        if (M16(A, 0x66) != 0) {
            pic_dispatch(B, M16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        return;
    case 4:
        anim_pair(A, B);
        if (MU16(A, 0x54) & 2)
            TASK_S8(task, state) = 5;
        return;
    case 5: {
        s32 idx;
        s16 code;
        if (flagB) {
            if (MU16(B, 0xCC) & 7)
                idx = 3;
            else
                idx = (s32)(rng_next() & 1) + 1;
        } else {
            idx = (s32)(rng_next() & 3);
        }
        code = M16(0x060862fc + idx * 2, 0);
        M16(A, 0x5C) = code;
        M16(B, 0x5C) = code + 1;
        M8(B, 0x41) = 1;
        M8(A, 0x41) = 1;
        TASK_S8(task, state) = idx + 6;
        anim_pair(A, B);
        if (M16(B, 0x56) < 4)
            M16(B, 0x56) = 4;
        return;
    }
    case 6:
    case 7:
        anim_pair(A, B);
        if (MU16(A, 0x54) & 2) {
            M32(B, 0x88) = 69;
            M32(A, 0x88) = 69;
            actor_decode_pose_direction(A);
            TASK_S8(task, state) = 10;
        }
        return;
    case 8:
        anim_pair(A, B);
        if (MU16(A, 0x54) & 2) {
            M32(B, 0x88) = 70;
            M32(A, 0x88) = 70;
            TASK_S8(task, state) = 10;
        }
        return;
    case 9:
        anim_pair(A, B);
        if (MU16(A, 0x54) & 2) {
            s32 off = ((u16)M16(A, 0x26) == 0x4000) ? -0x1000 : 0x1000;
            M32(A, 16) = M32(B, 16);
            M32(A, 8) = off + M32(B, 8);
            M32(B, 0x88) = 71;
            M32(A, 0x88) = 71;
            M16(A, 0x66) = M8(A, 0x4B);
            M16(B, 0x66) = M8(B, 0x4C);
            M16(A, 0x5C) = 0xDD;
            M16(B, 0x5C) = 0xDC;
            M16(A, 0x50) = 0;
            M8(B, 0x4C) = 0;
            M8(A, 0x4B) = 0;
            M8(A, 0x46) = 0;
            M8(B, 0x4A) = 0;
            M8(A, 0x4A) = 0;
            M8(B, 0x48) = 0;
            M8(A, 0x48) = 0;
            TASK_S32(task, handler) = 0;
        } else {
            s32 off = ((u16)M16(A, 0x26) == 0x4000) ? -0x1000 : 0x1000;
            M32(A, 8) += (off + M32(B, 8) - M32(A, 8)) >> 1;
            M32(A, 16) += (M32(B, 16) - M32(A, 16)) >> 1;
        }
        return;
    case 10:
        M32(A, 0x84) &= ~0x20000000;
        M32(A, 0x100) = M8(A, 0x4B);
        M32(B, 0x100) = M8(B, 0x4C);
        M16(A, 0x50) = 0;
        M8(A, 0x4B) = 0;
        M8(B, 0x4C) = 0;
        M8(A, 0x46) = 0;
        M8(B, 0x48) = 0;
        M8(A, 0x48) = 0;
        TASK_S32(task, handler) = 0;
        return;
    }
}

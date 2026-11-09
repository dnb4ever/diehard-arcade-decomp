#include "dh_work.h"
#include "dh.h"
/* 0x02226148 - tieup_task_handler: the two-actor "tie-up" (grapple/clash) task.
 *
 * Spawned by actor_grapple_handler (0x022485b8, state 1) via func_0220ff72.
 * Sibling of actor_clash_state (0x02226eb8). task->unk8 = "pair" record; the two
 * tied actors are A = pair->unk10 (r14) and B = pair->unk14 (r13). The task's own
 * progress is a 13-state FSM on the byte task->unk10 (0..12), dispatched through a
 * 13-entry braf jump table at 0x02226370 (braf @0x02226356, target = 0x2263 5a +
 * table[state]). States 0..12 map to 0x22638a,0x226450,0x2267c6,0x226912,0x2269ce,
 * 0x226a0e,0x226a48,0x226ac4,0x226b22,0x226b86,0x226bfc,0x226e48,0x226e72. State 11
 * falls through into state 12.
 *
 * Before the switch there is a large pre-dispatch block: it maintains each actor's
 * "combo descriptor" pointer (g_zone_records 0x06098734 + (unk34-1)*0x18, only for
 * player slots unk34<3), raises the shared spawn/priority gate (g_06098830 clamp 7,
 * mark via *g_0609882c), publishes the hit record (hitobj = (*g_0609882c)->unk8,
 * fields +0x7a/+0x7c/+0x7e / +0xae/+0xb0/+0xb2), evaluates the descriptor bit tests
 * that can force the teardown state 12, ticks A->unk50, and performs the per-actor
 * animation step via a function POINTER held in a register (r10 = animation_update,
 * 0x0222dc2c, loaded with mova+add). Each actor's current source object comes from
 * the per-slot pointer array g_060995B0[unk34].
 *
 * Indirect dispatches resolved:
 *   - r10 = animation_update @0x0222dc2c  (mova 0x22632c + 0x7900).  ANIM(e) below.
 *   - fn-ptr table g_060911BC (pose/reaction) indexed by mode flag g_0609979A,
 *     signature (actor,s32). POSE() below. (HWRAM table, runtime-indexed -> modelled
 *     as a faithful indexed call; no static entries exist in-cart.)
 *   - bsrf/braf tail targets (PIC): actor_script_step 0x022449b2 (returns 1=accept),
 *     func_02223e70, actor_behavior_fsm 0x02223ee0, func_0222ad7a, spawn func_0220ff0a,
 *     sound_queue_push 0x02215f6c. All resolved with target = bsrf+4+pool_delta.
 *
 * Standard traps observed: delay slots pre-load values used across branches
 * (e.g. r4=65 from the state-1 entry bf/s slot reused for several unk88 writes);
 * mov.b reads are sign-extended; shar is arithmetic >>1; the descriptor/slot
 * pointers are only valid for the paths that computed them (guarded by flagA).
 *
 * fidelity: HIGH (structure+data) - every state and the pre-dispatch hand-traced
 * against capstone disasm (0x226148..0x226eb8, 0xd70). The g_060911BC[mode] calls
 * are modelled as indexed indirect calls (RAM table, not resolvable in-cart);
 * everything else is a resolved named symbol. Field names are byte offsets. */

#define G8(a)  (*(s8  *)(uintptr_t)(a))
#define G16(a) (*(s16 *)(uintptr_t)(a))
#define G32(a) (*(s32 *)(uintptr_t)(a))
#define GP(a)  (*(void **)(uintptr_t)(a))

/* HWRAM globals */
#define ADDR_ENTITY_TBL   DH_ACTOR_SLOT_REGISTRY_ADDR_U   /* +8 count, +0xC id/flags, +0x2C ptr[] */
#define ADDR_SLOT_PTRS    DH_ACTOR_SLOT_TASKS_ADDR_U   /* = ENTITY_TBL+0x2C : g_060995B0[unk34] */
#define ADDR_ZONE_REC     DH_PLAYER_INPUT_RECORDS_ADDR_U   /* per-zone/combo descriptor, stride 0x18 */
#define ADDR_SCENE_FLAG   DH_PLAY_SESSION_MODE_ADDR_U
#define ADDR_REF_OBJ      DH_VIEW_TASK_ADDR_U
#define ADDR_SPAWN_GATE   DH_VIEW_REQUEST_PRIORITY_ADDR_U
#define ADDR_MODE_FLAG    DH_ACTOR_POSE_SET_ADDR_U   /* s16 index into g_060911BC */
#define ADDR_POSE_TAB     DH_ACTOR_POSE_HANDLERS_ADDR_U
#define ADDR_SLOT_ANIM    0x06096C74u   /* s32[] idx unk34 */
#define ADDR_GATE_TBL     0x06096E90u   /* s16[] idx unk34 */
#define ADDR_TIMER_TBL    0x0609975Cu   /* s32[] idx unk34-1 */
#define ADDR_REC7420      0x06097420u   /* s16[] idx unk34-1 */
#define ADDR_SOUND_ENABLE DH_SOUND_EFFECTS_ENABLED_ADDR_U
#define CHAIN_HIT_TASK    ((void *)(uintptr_t)0x0023F054u)

extern void  animation_update(void *src_obj, void *actor);        /* 0x0222dc2c */
extern s32   actor_script_step(void *actor);                      /* 0x022449b2 */
extern void  actor_reset_pose_history(void *actor);                          /* 0x02223e70 */
extern void  actor_behavior_fsm(void *actor);                     /* 0x02223ee0 */
extern void  actor_decode_pose_direction(void *actor);                          /* 0x0022ad7a */
extern void *task_spawn_actor_work(void *task, void *handler);            /* 0x0020ff0a */
extern void  sound_queue_push(u32 cmd);                           /* 0x02215f6c */

/* per-actor animation step: animation_update(g_060995B0[actor->unk34], actor) */
#define ANIM(e) \
    animation_update(GP(ADDR_SLOT_PTRS + (u32)((s32)M8((e), 0x34)) * 4u), (e))

/* pose/reaction call through g_060911BC[g_0609979A](actor, arg) */
#define POSE(e, arg) \
    ((void (*)(void *, s32))GP(ADDR_POSE_TAB + \
        (u32)((s32)G16(ADDR_MODE_FLAG)) * 4u))((e), (s32)(arg))

void tieup_task_handler(void *task)
{
    void *pair  = TASK_PTR(task, work);
    void *A     = MP(pair, 0x10);
    void *B     = MP(pair, 0x14);
    void *descA = (void *)0;        /* A's combo descriptor (only when flagA) */
    s32   flagA;                    /* A is a player slot (unk34 < 3) */
    void *objA_entry;               /* g_060995B0[A->unk34], the state switch arg */
    void *ref, *hitobj;
    s32   state;

    /* ---- pre-dispatch: actor A ---- */
    if ((s32)M8(A, 0x34) < 3) {
        descA = (void *)(uintptr_t)(ADDR_ZONE_REC +
                         (u32)(s8)(((s32)M8(A, 0x34) - 1) * 24));
        flagA = 1;
    } else {
        flagA = 0;
    }

    /* ---- pre-dispatch: actor B ---- */
    ref    = GP(ADDR_REF_OBJ);
    hitobj = MP(ref, 8);

    if ((s32)M8(B, 0x34) >= 3) {
        if (G32(ADDR_ENTITY_TBL + 8) == 1 &&
            G32(ADDR_SCENE_FLAG) == 1 &&
            G32(ADDR_SPAWN_GATE) >= 7) {
            G32(ADDR_SPAWN_GATE) = 7;
            M8(ref, 13) = 1;
            M16(hitobj, 0x7A) = M8(A, 0x34);
            M16(hitobj, 0x7C) = M8(B, 0x34);
            M16(hitobj, 0x7E) = 0;
        }
    } else {
        void *descB = (void *)(uintptr_t)(ADDR_ZONE_REC +
                               (u32)(s8)(((s32)M8(B, 0x34) - 1) * 24));
        s32 idx = ((s32)M8(B, 0x34) == 1) ? 2 : 1;

        if (G32(ADDR_ENTITY_TBL + 12 + idx * 4) == 0) {
            if (G32(ADDR_SCENE_FLAG) == 1 && G32(ADDR_SPAWN_GATE) >= 7) {
                G32(ADDR_SPAWN_GATE) = 7;
                M8(ref, 13) = 1;
                M16(hitobj, 0x7A) = M8(A, 0x34);
                M16(hitobj, 0x7C) = M8(B, 0x34);
                M16(hitobj, 0x7E) = 0;
            }
            M32(A, 0x84) |= 0x20000000;
        }
        /* descriptor-driven teardown trigger */
        if ((MU16(descB, 0) & 1) && (MU16(descB, 4) & 2) &&
            (MU16(descB, 0) & 4) && (s32)M16(B, 0x56) > 15 &&
            G16(ADDR_MODE_FLAG) == 0) {
            M16(A, 0x64) = 7;
            POSE(A, M16(A, 0x64));
            M32(A, 0x88) = 73;
            M32(B, 0x88) = 84;
            TASK_S8(task, state) = 12;
        }
    }

    /* ---- per-frame tick + handler hand-off ---- */
    M16(A, 0x50) = (s16)(M16(A, 0x50) + 1);
    if (M8(A, 0x48) != 0) {
        if (M8(A, 0x4A) != M8(B, 0x34)) {
            POSE(A, M16(A, 0x64));
            M32(B, 0x88) = 73;
            M32(A, 0x88) = 73;
            M8(B, 0x4A)  = M8(A, 0x4A);
            M16(B, 0x60) = M16(A, 0x60);
            M16(B, 0x62) = M16(A, 0x62);
            TASK_S8(task, state) = 12;
        }
        M8(A, 0x48) = 0;
    }

    /* ---- state dispatch ---- */
    objA_entry = GP(ADDR_SLOT_PTRS + (u32)((s32)M8(A, 0x34)) * 4u);
    state = (s32)TASK_U8(task, state);
    if ((u32)state >= 13)
        return;

    switch (state) {

    /* ---- state 0 (0x22638a) ---- */
    case 0:
        TASK_S8(task, state)++;
        if (M32(B, 0x88) == 45 && (s32)M8(A, 0x34) < 3) {
            s32 v = G32(ADDR_TIMER_TBL + ((s32)M8(A, 0x34) - 1) * 4);
            if (v > 4) {
                v >>= 1;
                M32(pair, 0x18) = v;
                G32(ADDR_TIMER_TBL + ((s32)M8(A, 0x34) - 1) * 4) =
                    M32(pair, 0x18);
            } else {
                M32(pair, 0x18) = v;
            }
            goto L22641c;
        }
        /* L2263d2 */
        if (flagA != 0) {
            if (M32(B, 0x38) & 0x6000) {
                M32(B, 0x88) = 43;
                M8(B, 0x42)  = 48;
                M32(pair, 0x18) =
                    G16(ADDR_REC7420 + ((s32)M8(A, 0x34) - 1) * 2);
                if ((s32)G16(ADDR_REC7420 + ((s32)M8(A, 0x34) - 1) * 2) > 1)
                    G16(ADDR_REC7420 + ((s32)M8(A, 0x34) - 1) * 2) =
                        (s16)(G16(ADDR_REC7420 + ((s32)M8(A, 0x34) - 1) * 2) - 1);
            } else {
                M32(pair, 0x18) = 30;
                M8(B, 0x42)     = 0;
            }
        } else {
            M32(pair, 0x18) = 30;
        }
    L22641c:
        M16(A, 0x50) = 0;
        M8(A, 0x46)  = 4;
        M8(B, 0x4A)  = M8(A, 0x34);
        M32(B, 0x08) = M32(A, 0x08) + M32(A, 0xC4);
        M32(B, 0x10) = M32(A, 0x10) + M32(A, 0xC8);
        return;

    /* ---- state 1 (0x226450) ---- */
    case 1:
        if (flagA != 0) {
            void *descP = descA;
            s16   dv    = M16(descP, 0);
            if (M8(A, 0x44) != 0 && ((u16)dv & 0x300))
                dv ^= 0x300;
            {
                s32 cnt = M32(pair, 0x18) - 1;
                M32(pair, 0x18) = cnt;
                if (cnt == 0) {
                    M16(A, 0x5C) = (s16)G32(ADDR_SLOT_ANIM +
                                            (s32)M8(A, 0x34) * 4);
                    M16(B, 0x5C) = (s16)G32(ADDR_SLOT_ANIM +
                                            (s32)M8(B, 0x34) * 4);
                    actor_reset_pose_history(A);
                    M32(A, 0x88) = 59;
                    if (M32(B, 0x88) == 43) {
                        M32(B, 0x88) = 59;
                    } else if (M8(B, 0x42) != 48) {
                        M32(B, 0x88) = 40;
                    }
                    TASK_S8(task, state) = 12;
                    goto apply_both;
                }
            }
            /* L2264d6 : cnt != 0 */
            if ((MU16(descP, 4) & 4) && M32(B, 0x88) == 43) {
                M16(A, 0x5C) = 0xB8;
                M8(A, 0x41)  = 1;
                M16(B, 0x5C) = 0xB9;
                M8(B, 0x41)  = 1;
                M32(B, 0x88) = 63;
                M32(A, 0x88) = 63;
                ANIM(A);
                ANIM(B);
                TASK_S8(task, state) = 3;
                goto apply_both;
            }
            /* L22653c */
            if (((u16)dv & 0x200) && M32(B, 0x88) == 43) {
                if (MU16(descP, 4) & 2) {
                    M16(A, 0x5C) = 0xC0;
                    M8(A, 0x41)  = 1;
                    M16(B, 0x5C) = 0xC1;
                    M8(B, 0x41)  = 1;
                    M32(B, 0x88) = 65;     /* r4 = 65 (state-1 entry delay slot) */
                    M32(A, 0x88) = 65;
                    ANIM(A);
                    ANIM(B);
                    TASK_S8(task, state) = 9;
                    goto apply_both;
                }
                /* L22659a */
                if (MU16(descP, 4) & 1) {
                    M16(A, 0x5C) = 0xBE;
                    M8(A, 0x41)  = 1;
                    M16(B, 0x5C) = 0xBF;
                    M8(B, 0x41)  = 1;
                    M32(B, 0x88) = 65;
                    M32(A, 0x88) = 65;
                    ANIM(A);
                    ANIM(B);
                    TASK_S8(task, state) = 8;
                }
                goto apply_both;
            }
            /* L2265ea */
            actor_behavior_fsm(A);
            goto apply_both;
        }
        /* flagA == 0 : sub-switch on A->unk42 (0x22665c) */
        switch ((s32)M8(A, 0x42)) {
        case 0:
        case 6:
            if (M8(A, 0x41) == 1) M8(B, 0x41) = 1;
            M16(B, 0x5C) = (s16)(M16(A, 0x5C) + 1);
            ANIM(A);
            ANIM(B);
            M32(B, 0x88) = 60;
            M32(A, 0x88) = 60;
            TASK_S8(task, state) = 2;
            return;
        case 1:
            M16(A, 0x5C) = 0xB8;
            M8(A, 0x41)  = 1;
            M16(B, 0x5C) = 0xB9;
            M8(B, 0x41)  = 1;
            M32(B, 0x88) = 63;
            M32(A, 0x88) = 63;
            ANIM(A);
            ANIM(B);
            TASK_S8(task, state) = 3;
            return;
        case 2:
            M16(A, 0x5C) = 0xC0;
            M8(A, 0x41)  = 1;
            M16(B, 0x5C) = 0xC1;
            M8(B, 0x41)  = 1;
            M32(B, 0x88) = 65;     /* r4 = 65 (entry delay slot) */
            M32(A, 0x88) = 65;
            ANIM(A);
            ANIM(B);
            TASK_S8(task, state) = 9;
            return;
        case 4:
        case 5:
        case 7:
            if (M8(A, 0x41) == 1) M8(B, 0x41) = 1;
            M16(B, 0x5C) = (s16)(M16(A, 0x5C) + 1);
            ANIM(A);
            ANIM(B);
            if (G32(ADDR_SCENE_FLAG) == 1 && G32(ADDR_SPAWN_GATE) >= 1) {
                void *ho;
                G32(ADDR_SPAWN_GATE) = 1;
                ho = MP(GP(ADDR_REF_OBJ), 8);
                M8(GP(ADDR_REF_OBJ), 13) = 1;
                M16(ho, 0xAE) = 0;
                M16(ho, 0xB0) = 0;
                M16(ho, 0xB2) = 0;
            }
            TASK_S8(task, state) = 10;
            return;
        default:
            return;
        }

    /* ---- state 2 (0x2267c6) ---- */
    case 2:
        if (flagA != 0)
            actor_reset_pose_history(A);
        else
            (void)actor_script_step(A);
        if (M8(A, 0x41) == 1) M8(B, 0x41) = 1;
        M16(B, 0x5C) = (s16)(M16(A, 0x5C) + 1);
        ANIM(A);
        ANIM(B);
        if (MU16(A, 0x66) != 0) {
            POSE(B, MU16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        if ((MU16(A, 0x54) & 2) == 0)
            return;
        actor_reset_pose_history(A);
        TASK_S8(task, state) = 12;
        if ((MU16(B, 0x54) & 0x200) != 0) {
            TASK_S8(task, state) = 11;
            return;
        }
        /* L226884 */
        if (MU16(A, 0x54) & 0x400) {
            M32(A, 0x84) = 0x800;
            M32(B, 0x84) = 0x80000;
            M32(A, 0x88) = 62;
            M32(B, 0x88) = 74;
            M16(A, 0x5C) = 0xA4;
            M16(B, 0x5C) = 0xA5;
            return;
        }
        /* L2268b0 */
        if (MU16(B, 0x54) & 0x100) {
            M32(B, 0x88) = 61;
            M32(B, 0x84) = 0x1000;
            actor_decode_pose_direction(B);       /* tail call (epilogue+braf in asm) */
            return;
        }
        if (MU16(B, 0x54) & 0x800) {
            M32(A, 0x88) = 59;
            M32(B, 0x88) = 66;
        } else if (MU16(B, 0x54) & 0x1000) {
            M32(A, 0x88) = 59;
            M32(B, 0x88) = 67;
        } else {
            M32(B, 0x88) = 60;
            M32(A, 0x88) = 60;
        }
        return;

    /* ---- state 3 (0x226912) ---- */
    case 3:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x58) < 11) {
            if (flagA == 0) {
                M16(A, 0x5C) = 0xBC;
                M16(B, 0x5C) = 0xBD;
                TASK_S8(task, state) = 5;
                return;
            }
            if (MU16(descA, 4) & 1) {
                M16(A, 0x5C) = 0xBA;
                M16(B, 0x5C) = 0xBB;
                TASK_S8(task, state) = 4;
                return;
            }
            if ((MU16(descA, 4) & 2) == 0)
                return;
            M16(A, 0x5C) = 0xBC;
            M16(B, 0x5C) = 0xBD;
            TASK_S8(task, state) = 5;
            return;
        }
        /* L22699c : A->unk58 >= 11 */
        if ((MU16(A, 0x54) & 2) == 0)
            return;
        M16(A, 0x5C) = (s16)G32(ADDR_SLOT_ANIM + (s32)M8(A, 0x34) * 4);
        M16(B, 0x5C) = (s16)G32(ADDR_SLOT_ANIM + (s32)M8(B, 0x34) * 4);
        TASK_S8(task, state) = 12;
        return;

    /* ---- state 4 (0x2269ce) ---- */
    case 4:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x58) != 12)
            return;
        M8(B, 0x41)  = 1;
        M8(A, 0x41)  = 1;
        M32(B, 0x88) = 64;
        M32(A, 0x88) = 64;
        M32(B, 0x84) = 0x80000;
        TASK_S8(task, state) = 6;
        return;

    /* ---- state 5 (0x226a0e) ---- */
    case 5:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x58) != 12)
            return;
        M8(B, 0x41)  = 1;
        M8(A, 0x41)  = 1;
        M32(B, 0x88) = 64;
        M32(A, 0x88) = 64;
        TASK_S8(task, state) = 7;
        return;

    /* ---- state 6 (0x226a48) ---- */
    case 6:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x66) != 0) {
            POSE(B, MU16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        if ((MU16(A, 0x54) & 2) == 0)
            return;
        M16(A, 0x5C) = (s16)G32(ADDR_SLOT_ANIM + (s32)M8(A, 0x34) * 4);
        M32(B, 0x84) = 0x1000;
        actor_decode_pose_direction(B);
        TASK_S8(task, state) = 12;
        return;

    /* ---- state 7 (0x226ac4) ---- */
    case 7:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x66) != 0) {
            POSE(B, MU16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        if (MU16(A, 0x58) < 16)
            return;
        M32(B, 0x88) = 74;
        M32(A, 0x88) = 74;
        M32(B, 0x84) = 0x80000;
        M32(A, 0x84) = 0x80000;
        TASK_S8(task, state) = 12;
        return;

    /* ---- state 8 (0x226b22) ---- */
    case 8:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x66) != 0) {
            POSE(B, MU16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        if ((MU16(A, 0x54) & 2) == 0)
            return;
        M16(A, 0x5C) = (s16)G32(ADDR_SLOT_ANIM + (s32)M8(A, 0x34) * 4);
        M32(B, 0x84) = 0x1000;
        actor_decode_pose_direction(B);
        TASK_S8(task, state) = 12;
        return;

    /* ---- state 9 (0x226b86) ---- */
    case 9:
        animation_update(objA_entry, A);
        ANIM(B);
        if (MU16(A, 0x66) != 0) {
            POSE(B, MU16(A, 0x66));
            M16(A, 0x66) = 0;
        }
        if (MU16(A, 0x58) < 16)
            return;
        M32(B, 0x88) = 74;
        M32(A, 0x88) = 74;
        M32(B, 0x84) = 0x80000;
        M32(A, 0x84) = 0x80000;
        TASK_S8(task, state) = 12;
        return;

    /* ---- state 10 (0x226bfc) ---- */
    case 10: {
        s32 sel = (s32)M8(A, 0x42) - 4;
        M16(B, 0x5C) = (s16)(M16(A, 0x5C) + 1);
        ANIM(A);
        ANIM(B);
        switch (sel) {
        case 0:
            if (MU16(A, 0x66) != 0) {
                POSE(B, MU16(A, 0x66));
                M16(A, 0x66) = 0;
            }
            if (actor_script_step(A) != 1)
                return;
            M32(A, 0x88) = 74;
            M16(B, 0x5C) = (s16)(M16(A, 0x5C) + 1);
            M16(B, 0x62) = (s16)(M8(A, 0x44) != 0 ? 0xC000 : 0x4000);
            M32(B, 0x88) = sel + 75;
            if (G32(ADDR_SCENE_FLAG) == 1 && G32(ADDR_SPAWN_GATE) >= 1) {
                void *ho;
                G32(ADDR_SPAWN_GATE) = 1;
                ho = MP(GP(ADDR_REF_OBJ), 8);
                M8(GP(ADDR_REF_OBJ), 13) = 1;
                M16(ho, 0xAE) = 0;
                M16(ho, 0xB0) = 0;
                M16(ho, 0xB2) = 0;
            }
            TASK_S8(task, state) = 12;
            return;
        case 1: {
            if (MU16(A, 0x66) != 0) {
                void *nt = task_spawn_actor_work(task, CHAIN_HIT_TASK);
                void *ne = MP(nt, 8);
                M16(A, 0x66) = 0;
                M8(ne, 0x34)  = M8(A, 0x34);
                M16(ne, 0x5A) = M16(B, 0x5A);
                M8(ne, 0x4C)  = M8(B, 0x34);
                if (G32(ADDR_SOUND_ENABLE) != 0)
                    sound_queue_push(0xA0140CFFu);
            }
            if (actor_script_step(A) != 1)
                return;
            M32(A, 0x88) = 74;
            M32(B, 0x84) = 0x80000;
            M32(B, 0x88) = 74;
            POSE(B, 10);
            if (G32(ADDR_SCENE_FLAG) == 1 && G32(ADDR_SPAWN_GATE) >= 1) {
                void *ho;
                G32(ADDR_SPAWN_GATE) = 1;
                ho = MP(GP(ADDR_REF_OBJ), 8);
                M8(GP(ADDR_REF_OBJ), 13) = 1;
                M16(ho, 0xAE) = 0;
                M16(ho, 0xB0) = 0;
                M16(ho, 0xB2) = 0;
            }
            TASK_S8(task, state) = 12;
            return;
        }
        case 3:
            if (MU16(A, 0x66) != 0) {
                POSE(B, MU16(A, 0x66));
                M16(A, 0x66) = 0;
            }
            if (actor_script_step(A) != 1)
                return;
            M32(A, 0x88) = 74;
            M32(B, 0x84) = 0x80000;
            M32(B, 0x88) = 74;
            POSE(B, 10);
            if (G32(ADDR_SCENE_FLAG) == 1 && G32(ADDR_SPAWN_GATE) >= 1) {
                void *ho;
                G32(ADDR_SPAWN_GATE) = 1;
                ho = MP(GP(ADDR_REF_OBJ), 8);
                M8(GP(ADDR_REF_OBJ), 13) = 1;
                M16(ho, 0xAE) = 0;
                M16(ho, 0xB0) = 0;
                M16(ho, 0xB2) = 0;
            }
            TASK_S8(task, state) = 12;
            return;
        default:
            return;
        }
    }

    /* ---- state 11 (0x226e48) : falls through into state 12 ---- */
    case 11:
        M32(B, 0x88) = 68;
        M32(A, 0x88) = 68;
        M16(A, 0x5C) = 0xDC;
        M16(B, 0x5C) = 0xDD;
        M8(B, 0x41)  = 1;
        M8(A, 0x41)  = 1;
        M16(A, 0x66) = M8(A, 0x4B);
        M16(B, 0x66) = M8(B, 0x4C);
        /* fall through */

    /* ---- state 12 (0x226e72) : teardown ---- */
    case 12:
        M32(A, 0x84) &= (s32)0xDFFFFFFF;
        M32(A, 0x100) = (s32)M8(A, 0x4B);
        M32(B, 0x100) = (s32)M8(B, 0x4C);
        M16(A, 0x50) = 0;
        M8(A, 0x4B)  = 0;
        M8(B, 0x4C)  = 0;
        M8(A, 0x46)  = 0;
        M8(B, 0x48)  = 0;
        M8(A, 0x48)  = 0;
        TASK_S32(task, handler) = 0;
        return;

    default:
        return;
    }

    /* shared tail (L2265f0): gate on g_06096E90[A->unk34]; if clear, step both
       actors and settle to state 2 (unless a caller already set a sticky state
       and the gate kept it by early-return). */
apply_both:
    if (G16(ADDR_GATE_TBL + (s32)M8(A, 0x34) * 2) != 0)
        return;
    if (M8(A, 0x41) == 1)
        M8(B, 0x41) = 1;
    M16(B, 0x5C) = (s16)(M16(A, 0x5C) + 1);
    ANIM(A);
    ANIM(B);
    M32(B, 0x88) = 60;
    M32(A, 0x88) = 60;
    TASK_S8(task, state) = 2;
}

/* fidelity: equivalence-faithful. 13-state FSM (task->unk10) + pre-dispatch fully
   hand-traced vs capstone over 0x226148..0x226eb8 (0xd70); braf table @0x226370
   resolved to states 0..12 (11 falls through to 12). All PIC bsrf/braf resolved to
   named symbols (animation_update 0x22dc2c, actor_script_step 0x2449b2,
   func_02223e70, actor_behavior_fsm 0x223ee0, func_0222ad7a, spawn func_0220ff0a,
   sound_queue_push 0x215f6c). Only unresolved detail: the pose/reaction calls
   g_060911BC[g_0609979A](actor,arg) go through a HWRAM RAM fn-ptr table with a
   runtime index, modelled as a faithful indexed indirect call (table not in cart). */


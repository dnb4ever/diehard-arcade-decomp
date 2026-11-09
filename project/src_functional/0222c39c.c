/* player_handle_grab_throw_commands [0x0222c39c]: Input/state gates choose grabbing, being grabbed and throw sequence. */
#include "dh.h"
/* 0x0222c39c  actor_combat_cmd_handler - main grab/throw command handler.
 * Decides whether an actor grabs, is grabbed, or launches a throw sequence.
 *
 * ABI: actor = r4; r5 unused; sel = r6 (only ever forwarded to a 1-arg tail and
 *   therefore dead in a faithful rendering). Single function [0x0222c39c,0x22ca40)
 *   (rts @0x22ca3e), split out of the 0x22c39c pool group (bounded --end 0x22ca42).
 *
 * Shape:
 *   rec5 = 0x06098734 + (s8)((unk34-1)*0x18)  (per-actor combat record)
 *   if (actor->unk48) { func_0222adaa(actor); return; }          // busy -> tail
 *   slot4C = actor->unk4C;  angidx = g_06096E90[unk34] (s16).
 *   if (slot4C == 0):                                            // not yet grabbed
 *      - (optional) scan command queue slots 0..7 for a state to enter via
 *        actor_state_select; on >=0 set unk43/unk84 and return.
 *      - block_100: housekeeping (func_0222c220/func_02223ee0/func_0222bfc2),
 *        then scan other actors (slots 1..6) for a grab candidate; on a match set
 *        up the grab link on both actors and return.
 *   else (slot4C != 0):                                          // being grabbed
 *      - snap to the grabber, run the throw-alignment switch on unk88, then
 *        func_0223a9d0(actor, 3).
 *
 * m2c corrections (braf_resolve is authoritative; the --asm rewriter and the
 * monsters2 note both mis-assigned these deltas):
 *   @0x22c434 bsrf -> func_02223e70 (NOT func_0223a9d0): the M2C_ERROR slot in
 *            the slot4C!=0 / angidx!=0 branch. 1-arg (02223e70.c).
 *   @0x22c876 bsrf -> func_02223ee0 (NOT func_0223a9d0): block_100's
 *            `if (!(unk84 & 0x400000))` call. 1-arg (02223ee0.c).
 *   @0x22c7a2 bsrf -> func_0223a9d0 (NOT func_02223e70): the end-of-else call,
 *            r5=3 so a1=(void*)3. 2-arg (0223a9d0.c: void*,void*).
 *   @0x22c9ac bsrf -> store_angle_between (NOT func_02223ee0): the loop_115
 *            5-arg angle probe (022107dc.c), result's sign bit picks facing.
 *   func_0222adaa / actor_state_select have fewer real params than the call-site
 *   register setup (1-arg / 3-arg per 0222adaa.c, 0239ce8.c); extra regs spurious.
 *
 * fidelity: HIGH - every computed call resolved via braf_resolve and matched to
 *   the defining TU's real signature; switch literals verified vs fn.py --asm.
 *   tst #imm all < 0x80 (no sign-extend trap); the 0x22c3e8 shll-r0 bsrf is the
 *   0x0003xxxx pool phantom and is not a real branch. */

extern void actor_dispatch_pose_and_type(void *entity);                  /* 0x0222adaa (1 arg) */
extern s32  actor_state_select(void *actor, s32 r5, s32 sel); /* 0x02239ce8 */
extern s32  player_try_special_move(void *actor);                   /* 0x0222c220 */
extern void player_update_combat_input(void *actor);                   /* 0x02223ee0 */
extern void player_scan_grab_target(void *actor);                   /* 0x0222bfc2 (extern) */
extern void actor_reset_pose_history(void *actor);                   /* 0x02223e70 */
extern void actor_commit_effect_handoffs(void *ent, void *a1);           /* 0x0223a9d0 */
extern void store_angle_between(s32 x0, s32 y0, s32 x1, s32 y1, s16 *out); /* 0x022107dc */

void player_handle_grab_throw_commands(void *actor, s32 r5_unused, s32 sel) {
    s8    actor_slot_index  = M8(actor, ACTOR_SLOT_ID);
    void *rec5   = (void *)(uintptr_t)(DH_PLAYER_INPUT_RECORDS_ADDR + (s8)((actor_slot_index - 1) * 0x18));
    s8    slot4C;
    s16   angidx;
    s32   i;

    (void)r5_unused;
    (void)sel;

    if (M8(actor, 0x48) != 0) {
        actor_dispatch_pose_and_type(actor);                 /* 0x22c3d2 tail */
        return;
    }

    slot4C = M8(actor, 0x4C);
    angidx = M16(0x06096E90, actor_slot_index * 2);

    if (slot4C == 0) {
        if ((M32(actor, 0x84) & 0xD) && ((u16)angidx == 0) && (MU16(rec5, 0x4) & 1)) {
            for (i = 0; i < 8; i++) {
                s32 off = i * 4;
                if (MP(0x060E0F0C, off) != 0 &&
                    M32(0x060E0F2C, off) == 1 &&
                    M32(MP(0x060E0F4C, off), 0x10) == 1) {
                    if (M32(0x060E0F6C, ((s32)actor_slot_index << 5) + off) < 0xA900) {
                        s32 r = actor_state_select(actor, M32(MP(0x060E0F4C, off), 0x8), i);
                        if (r >= 0) {
                            M8(actor, 0x43)   = (s8)(r + 0x21);
                            M32(actor, 0x84) |= 0x20000000;
                            return;
                        }
                        break;                /* -> block_100 */
                    }
                }
            }
        }

        /* block_100 */
        if (player_try_special_move(actor) != 0) {
            return;
        }
        if (!(M32(actor, 0x84) & 0x400000)) {
            player_update_combat_input(actor);             /* 0x22c876 */
        }
        if ((M32(actor, 0x84) & 4) && M32(actor, 0x88) == 1 && M8(actor, 0x45) <= 1) {
            player_scan_grab_target(actor);
            return;
        }
        if (M8(actor, 0x45) != 0) {
            return;
        }
        if (!(M32(actor, 0x84) & 0x100)) {
            return;
        }
        if (M32(actor, 0x88) != 0x14) {
            return;
        }

        for (i = 1; i < 7; i++) {             /* scan other actors, slots 1..6 */
            s32   off = i * 4;
            s32   depth, of84, d;
            void *other;
            s16   scratch[2];

            if (M32(DH_ACTOR_SLOT_IDS_ADDR, off) == 0) {
                continue;
            }
            depth = M32(0x060995D0, ((s32)actor_slot_index << 5) + off);
            if (depth >= 0x12100 || depth <= 0) {
                continue;
            }
            other = MP(MP(DH_ACTOR_SLOT_TASKS_ADDR, off), 0x8);
            if (!(M8(other, 0x34) > 2 || M16(DH_ACTOR_POSE_SET_ADDR, 0) != 0)) {
                continue;
            }
            of84 = M32(other, 0x84);
            if (!((of84 & 0x40D) && !(of84 & 0x237FBB00) && M8(other, 0x48) == 0 &&
                  !(M32(other, 0x38) & 0x9E00) && M8(other, 0x4E) == 0)) {
                continue;
            }
            d = M32(actor, 0x10) - M32(other, 0x10);
            if (d < 0) {
                d = -d;
            }
            if (d >= 0x4000) {
                continue;
            }
            store_angle_between(M32(actor, 0x8), M32(actor, 0x10),
                                M32(other, 0x8), M32(other, 0x10), scratch); /* 0x22c9ac */
            if (M8(actor, 0x44) == (s8)((u16)scratch[0] >> 15)) {
                continue;
            }
            /* grab link set-up */
            M8(actor, 0x4B)  = (s8)M8(other, 0x34);
            M8(other, 0x4C)  = (s8)M8(actor, ACTOR_SLOT_ID);
            M32(other, 0xE0) = 0x2000;
            M32(other, 0x84) = 0x40000;
            M32(other, 0x88) = 0x2C;
            M32(actor, 0x84) = 0x20000;
            M8(actor, 0x41)  = 1;
            M8(actor, 0x43)  = 0xF;
            if ((u16)M16(actor, 0x5A) == 0x76) {
                M16(actor, ACTOR_ANIM_REQUEST) = 0x78;
            } else {
                M16(actor, ACTOR_ANIM_REQUEST) = 0x7D;
            }
            return;
        }
        return;
    }

    /* slot4C != 0 : this actor is being grabbed / thrown */
    {
        void *other = MP(MP(DH_ACTOR_SLOT_TASKS_ADDR, slot4C * 4), 0x8);

        if ((u16)angidx != 0) {
            actor_reset_pose_history(actor);             /* 0x22c434 */
            M16(actor, 0x52) = 0;
        }
        M8(actor, 0x41)  = 1;
        M32(actor, 0x10) = M32(other, 0x10) + M32(actor, 0xC8);
        M32(actor, 0x78) = 0;
        M32(actor, 0x7C) = 0;
        M32(actor, 0x80) = 0;

        switch (M32(actor, 0x88)) {
        case 0x4B:
            M8(actor, 0x43) = 0x16;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x275;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFEF5C3 : 0x10A3D);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x274;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFEF5C3 : 0x10A3D);
            }
            break;
        case 0x4C:
            M8(actor, 0x43) = 0x16;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x24C;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFEE667 : 0x11999);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x24B;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFEE667 : 0x11999);
            }
            break;
        case 0x4D:
            M8(actor, 0x43) = 0x16;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x21E;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF199A : 0xE666);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x21D;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF199A : 0xE666);
            }
            break;
        case 0x4E:
            M8(actor, 0x43) = 0x16;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x27C;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFEF3B7 : 0x10C49);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x27B;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFEF3B7 : 0x10C49);
            }
            break;
        case 0x2C:
            M8(actor, 0x43) = 0x10;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M16(actor, ACTOR_ANIM_REQUEST) = 0x79;
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF3C74 : 0xC38C);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x77;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF3C74 : 0xC38C);
            }
            break;
        case 0x2B:
            M8(actor, 0x43) = 0x16;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M16(actor, ACTOR_ANIM_REQUEST) = 0x95;
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF5652 : 0xA9AE);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x93;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF3C74 : 0xC38C);
            }
            break;
        case 0x2D:
            M8(actor, 0x43) = 0x16;
            if (M8(actor, 0x44) == M8(other, 0x44)) {
                M16(actor, ACTOR_ANIM_REQUEST) = 0x2E5;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF5652 : 0xA9AE);
            } else {
                M16(actor, OBJ_ANGLE_Y) = (s16)(M16(actor, OBJ_ANGLE_Y) + 0x8000);
                M8(actor, 0x44)  = (s8)(M8(actor, 0x44) ^ 1);
                M16(actor, ACTOR_ANIM_REQUEST) = 0x2E2;
                M32(actor, 0x8)  = M32(other, 0x8) +
                    (M8(other, 0x44) != 0 ? (s32)0xFFFF3C74 : 0xC38C);
            }
            break;
        default:
            break;
        }

        if (!(M32(actor, 0x8C) & 0xFFFFFF)) {
            return;
        }
        M32(actor, 0x74) = 0;
        M16(actor, 0x62) = (M8(actor, 0x44) != 0) ? (s16)0xC000 : 0x4000;
        M16(actor, 0x60) = 0x2000;
        M8(actor, 0x45)  = 0;
        actor_commit_effect_handoffs(actor, (void *)3);      /* 0x22c7a2 */
    }
}

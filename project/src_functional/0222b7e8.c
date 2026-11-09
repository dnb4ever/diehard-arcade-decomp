/* player_select_motion_animation [0x0222b7e8]: Chooses animation and continuation from motion/action flags and updates linked effect handoffs. */
#include "dh.h"
/* 0x0222b7e8  actor_anim_select_main  (entity=r4)
 *
 * The full-featured member of the anim-selector family (see 0x0222b0fe header);
 * siblings 0x0222b0fe and 0x0222b1e2 tail-call here for their rich-motion cases.
 * Classifies the motion flags unk84 and picks the attack-animation, choosing
 * between two top-level anim table banks (g_060866f8 "normal" / g_0608670c
 * "alt") by a per-case selector, then tail-calls the follow-up handler
 * 0x022160a0.
 *
 * Cases on unk84 (priority order):
 *   0x2000  : group=2, sub=0, unk88=7, unk43=4, bank=normal, flags kept; run 0x0222acf8.
 *   0x81000 : newflag=0x2000, group=3, unk88=0, unk43=4, bank=normal,
 *             sub=((u8)unk49>5)?1:0; run 0x0222acf8.
 *   0x60000 : attack-box A; newflag=0x1000, group=1, sub=0, unk88=0, unk43=3,
 *             bank=normal; if g_06096e90[unk34] also attack-box B. (no 0x0222acf8)
 *   0x318000: newflag=0x2000, group=3, unk88=0, unk43=4, bank=normal,
 *             sub=((u8)unk49>5)?1:0; run 0x0222acf8.
 *   0x4000  : unk5C = g_06086304[unk4d & 1]; return immediately (no tail call).
 *   !0x400FFD: diagnostic path (unexpected flags) -> see below; group=0, sub=1,
 *             bank=normal, flags kept.
 *   else (0x400FFD set): combo/pressure branch, split on whether unk46==6:
 *       unk46==6 : attack-box B if flags&0x400; group=4, unk88=0.
 *       else     : attack-box A (flags&0x30) or B (flags&0x400); group=0, unk88=0;
 *                  sub = unk49? (((u8)unk49>5)?1:0) : 2.
 *     both share the combo gate: if unk64 in (0,0x10) and unk56!=0 and
 *     ++unk10C < 6 -> newflag=0x800, bank=alt, unk43=2; else newflag=0x1000,
 *     bank=normal, unk43=3; for unk46==6, sub=((u8)unk49>5)?1:0.
 * Shared tail: unk84=newflag; hit-reaction fx on unk8C&0xFFFFFF; unk46=0;
 *   unk5C = bank[group][sub][dir*4+col]; tail-call 0x022160a0(entity).
 *
 * Fields: unk34/43/44/45/46/49/4D byte, unk56/5C/60/62/64 word,
 *   unk84/88/8C/10C long.
 * Branches resolved via braf_resolve (bsrf -> 0x227704, 0x223e70, 0x23a9d0;
 * bsr -> 0x0222acf8; tail braf -> 0x022160a0). The disassembler renders several
 * literal-pool longs in this body as phantom bsr/mov instructions; those are the
 * resolved PC-relative deltas, not code. Cross-checked vs fn.py. fidelity: HIGH
 * except the diagnostic-path indirect call (ABI noted inline). */
#define R16A(a) (*(s16 *)(uintptr_t)(a))
#define R32A(a) (*(s32 *)(uintptr_t)(a))
#define PA(a)   (*(void **)(uintptr_t)(a))

extern void actor_resolve_facing(void *entity);
extern void actor_reset_pose_history(void *entity);
extern void actor_compute_motion_components(void *entity);
extern void actor_commit_effect_handoffs(void *entity, s32 a1);
extern void audio_queue_actor_cue(void *entity);           /* follow-up handler (tail call) */

static s16 anim_tbl(uintptr_t base, s32 group, s32 sub, u32 dir, u32 col) {
    void *tbl = PA(base + group * 4);
    void *row = PA((uintptr_t)tbl + sub * 4);
    return R16A((uintptr_t)row + (dir * 4 + col) * 2);
}

/* combo gate shared by the two 0x400FFD sub-branches: returns 1 (short/alt) or
 * 0 (long/normal) and advances the combo counter unk10C. */
static s32 combo_gate(void *entity) {
    s16 w = M16(entity, 0x64);
    return (w < 0x10) && (w != 0) && (M16(entity, 0x56) != 0) &&
           ((M32(entity, 0x10C) = M32(entity, 0x10C) + 1) < 6);
}

void player_select_motion_animation(void *entity) {
    s32 flags = M32(entity, 0x84);
    s32 newflag = flags;
    s32 group;
    s32 sub;
    uintptr_t bank;
    u32 dir;
    u32 col;

    if (flags & 0x2000) {
        group = 2;
        sub = 0;
        M32(entity, 0x88) = 7;
        M8(entity, 0x43) = 4;
        bank = 0x060866F8;
        actor_compute_motion_components(entity);
    } else if (flags & 0x81000) {
        newflag = 0x2000;
        group = 3;
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 4;
        sub = ((s32)(u8)M8(entity, 0x49) > 5) ? 1 : 0;
        bank = 0x060866F8;
        actor_compute_motion_components(entity);
    } else if (flags & 0x60000) {
        actor_resolve_facing(entity);
        newflag = 0x1000;
        group = 1;
        sub = 0;
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 3;
        bank = 0x060866F8;
        if (R16A(0x06096E90 + M8(entity, ACTOR_SLOT_ID) * 2) != 0)
            actor_reset_pose_history(entity);
    } else if (flags & 0x318000) {
        newflag = 0x2000;
        group = 3;
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 4;
        sub = ((s32)(u8)M8(entity, 0x49) > 5) ? 1 : 0;
        bank = 0x060866F8;
        actor_compute_motion_components(entity);
    } else if (flags & 0x4000) {
        M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x06086304 + (M8(entity, 0x4D) & 1) * 2);
        return;
    } else if (!(flags & 0x400FFD)) {
        /* diagnostic path: unexpected motion-flag combination. asm sets two
         * globals then makes an indirect call through the function pointer at
         * 0x060902A0 with r4=0x2000 and two stack pushes (unk84, then
         * 0x06087EE8), r5-r7 left unset -> almost certainly a varargs logger.
         * Modelled with the observed operands; callee ABI is unverified. */
        R32A(DH_UI_CURSOR_COLUMN_ADDR) = 5;
        R32A(DH_UI_CURSOR_ROW_ADDR) = 14;
        (*(void (**)(s32, void *, u32))(uintptr_t)0x060902A0)(
            0x2000, (void *)(uintptr_t)0x06087EE8, MU32(entity, 0x84));
        group = 0;
        sub = 1;
        bank = 0x060866F8;
    } else if (M8(entity, 0x46) == 6) {
        if (flags & 0x400)
            actor_reset_pose_history(entity);
        M32(entity, 0x88) = 0;
        group = 4;
        sub = ((s32)(u8)M8(entity, 0x49) > 5) ? 1 : 0;
        if (combo_gate(entity)) {
            newflag = 0x800;
            M8(entity, 0x43) = 2;
            bank = 0x0608670C;
        } else {
            newflag = 0x1000;
            M8(entity, 0x43) = 3;
            bank = 0x060866F8;
        }
    } else {
        if (flags & 0x30)
            actor_resolve_facing(entity);
        else if (flags & 0x400)
            actor_reset_pose_history(entity);
        M32(entity, 0x88) = 0;
        group = 0;
        if (M8(entity, 0x49) != 0)
            sub = ((s32)(u8)M8(entity, 0x49) > 5) ? 1 : 0;
        else
            sub = 2;
        if (combo_gate(entity)) {
            newflag = 0x800;
            M8(entity, 0x43) = 2;
            bank = 0x0608670C;
        } else {
            newflag = 0x1000;
            M8(entity, 0x43) = 3;
            bank = 0x060866F8;
        }
    }

    M32(entity, 0x84) = newflag;
    col = (u16)(M16(entity, 0x60) + 0x4000) >> 14;
    dir = (u16)M16(entity, 0x62) >> 13;
    if (M8(entity, 0x44) != 0)
        dir = (dir + 4) & 7;
    if (MU32(entity, 0x8C) & 0xFFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
    M8(entity, 0x46) = 0;
    M16(entity, ACTOR_ANIM_REQUEST) = anim_tbl(bank, group, sub, dir, col);

    audio_queue_actor_cue(entity);
}

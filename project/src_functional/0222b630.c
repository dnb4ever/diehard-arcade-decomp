/* player_select_knockback_animation [0x0222b630]: Knockback variant uses flat fallback animation table without RNG. */
#include "dh.h"
/* 0x0222b630  actor_anim_select_knockback  (entity=r4)
 *
 * Sibling anim selector (see 0x0222b0fe header). Identical in shape to
 * 0x0222b42c except the fall-through case uses a flat 1-D table (g_06086728) and
 * draws no rng. group index defaults to 2. Cases on motion flags unk84:
 *   unk84 & 0x82000  : newflag=0x2000, unk88=7, unk43=4, sub=0, group=2;
 *                      run physics helper 0x0222acf8; 3-level table lookup.
 *   unk84 & 0x318000 : newflag=0x2000, unk88=0, unk43=4, group=3,
 *                      sub=((u8)unk49>5)?1:0; run 0x0222acf8; 3-level table.
 *   unk84 & 0x60000  : attack-box A; newflag=0x1000, unk88=0, unk43=3, group=1,
 *                      sub=0; if g_06096e90[unk34] also attack-box B; 3-level table.
 *   otherwise        : attack-box A (flags&0x30) or B (flags&0x400); newflag=0x800,
 *                      unk88=0, unk43=2; anim = g_06086728[facing-hi ^ unk44].
 * Shared tail: unk5C=sel; unk84=newflag; unk46=0; hit-reaction fx on
 * unk8C&0xFFFFFF; publishes g_06099780[unk34-1]=g_060987bc at entry and exit.
 *
 * Fields: unk34/43/44/45/46/49 byte, unk5C/60/62 word, unk84/88/8C long.
 * Branches resolved via braf_resolve (bsrf -> 0x227704, 0x223e70, 0x23a9d0;
 * bsr -> 0x0222acf8). Cross-checked vs fn.py. fidelity: HIGH. */
#define R16A(a) (*(s16 *)(uintptr_t)(a))
#define R32A(a) (*(s32 *)(uintptr_t)(a))
#define PA(a)   (*(void **)(uintptr_t)(a))

extern void actor_resolve_facing(void *entity);
extern void actor_reset_pose_history(void *entity);
extern void actor_compute_motion_components(void *entity);
extern void actor_commit_effect_handoffs(void *entity, s32 a1);

static s16 anim_tbl(uintptr_t base, s32 group, s32 sub, u32 dir, u32 col) {
    void *tbl = PA(base + group * 4);
    void *row = PA((uintptr_t)tbl + sub * 4);
    return R16A((uintptr_t)row + (dir * 4 + col) * 2);
}

void player_select_knockback_animation(void *entity) {
    s32 flags = M32(entity, 0x84);
    s32 group = 2;
    s32 sub = 0;
    s32 newflag;
    s16 sel;

    R32A(0x06099780 + (M8(entity, ACTOR_SLOT_ID) - 1) * 4) = R32A(DH_FRAME_TICK_ADDR);

    if (flags & 0x3FA000) {
        /* 0x82000 | 0x318000 | 0x60000 == 0x3FA000; sequential sub-tests below
         * preserve the asm's priority order -> 3-level table g_060866f8[group][sub] */
        if (flags & 0x82000) {
            newflag = 0x2000;
            M32(entity, 0x88) = 7;
            M8(entity, 0x43) = 4;
            actor_compute_motion_components(entity);
        } else if (flags & 0x318000) {
            newflag = 0x2000;
            M32(entity, 0x88) = 0;
            M8(entity, 0x43) = 4;
            group = 3;
            sub = ((s32)(u8)M8(entity, 0x49) > 5) ? 1 : 0;
            actor_compute_motion_components(entity);
        } else {
            actor_resolve_facing(entity);
            newflag = 0x1000;
            M32(entity, 0x88) = 0;
            M8(entity, 0x43) = 3;
            group = 1;
            if (R16A(0x06096E90 + M8(entity, ACTOR_SLOT_ID) * 2) != 0)
                actor_reset_pose_history(entity);
        }
        {
            u32 col = (u16)(M16(entity, 0x60) + 0x4000) >> 14;
            u32 dir = (u16)M16(entity, 0x62) >> 13;
            if (M8(entity, 0x44) != 0)
                dir = (dir + 4) & 7;
            sel = anim_tbl(0x060866F8, group, sub, dir, col);
        }
    } else {
        u32 dir;
        if (flags & 0x30)
            actor_resolve_facing(entity);
        else if (flags & 0x400)
            actor_reset_pose_history(entity);
        M32(entity, 0x88) = 0;
        newflag = 0x800;
        M8(entity, 0x43) = 2;
        dir = (u16)M16(entity, 0x62) >> 15;
        if (M8(entity, 0x44) != 0)
            dir ^= 1;
        sel = R16A(0x06086728 + dir * 2);
    }

    M16(entity, ACTOR_ANIM_REQUEST) = sel;
    M32(entity, 0x84) = newflag;
    M8(entity, 0x46) = 0;
    if (MU32(entity, 0x8C) & 0xFFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }

    R32A(0x06099780 + (M8(entity, ACTOR_SLOT_ID) - 1) * 4) = R32A(DH_FRAME_TICK_ADDR);
}

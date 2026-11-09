/* player_select_launch_animation [0x0222b2a8]: Launch variant classifies motion, chooses tables and seeds airborne state. */
#include "dh.h"
/* 0x0222b2a8  actor_anim_select_launch  (entity=r4)
 *
 * Sibling anim selector (see 0x0222b0fe header). Handles the "launched / aerial
 * throw" variant. Two cases on the motion flags unk84:
 *   unk84 & 0x60000 : attack-box A; unk88=0, unk43=3, group=1, newflag=0x1000;
 *                     if the per-actor gate g_06096e90[unk34] is set also fire
 *                     attack-box B.
 *   otherwise       : attack-box A (flags&0x30) or B (flags&0x400); unk88=0,
 *                     unk43=4, group=3, newflag=0x2000; run the shared
 *                     velocity/physics helper 0x0222acf8; bump unkC8 by 0x800;
 *                     set horizontal push unk78 by facing sign and unk80=-0x3000.
 * Common tail: unk84=newflag; pick the anim from the 3-level table g_060866f8
 * [group][0] by (dir,col) derived from unk62/unk60/unk44; unk46=0; hit-reaction
 * fx on unk8C&0xFFFFFF; combo SFX 0xA01200FF when g_060E0DDC set; publish
 * g_06099780[unk34-1] = g_060987bc.
 *
 * Fields: unk34/43/44/45/46 byte, unk5C/60/62 word, unk78/80/84/88/8C/C8 long.
 * Branches resolved via braf_resolve (bsrf -> 0x227704, 0x223e70, 0x23a9d0,
 * 0x215f6c; bsr -> 0x0222acf8). Cross-checked vs fn.py. fidelity: HIGH. */
#define R16A(a) (*(s16 *)(uintptr_t)(a))
#define R32A(a) (*(s32 *)(uintptr_t)(a))
#define PA(a)   (*(void **)(uintptr_t)(a))

extern void actor_resolve_facing(void *entity);
extern void actor_reset_pose_history(void *entity);
extern void actor_compute_motion_components(void *entity);
extern void actor_commit_effect_handoffs(void *entity, s32 a1);
extern void sound_queue_push(u32 cmd);

static s16 anim_tbl(uintptr_t base, s32 group, s32 sub, u32 dir, u32 col) {
    void *tbl = PA(base + group * 4);
    void *row = PA((uintptr_t)tbl + sub * 4);
    return R16A((uintptr_t)row + (dir * 4 + col) * 2);
}

void player_select_launch_animation(void *entity) {
    s32 flags = M32(entity, 0x84);
    s32 group;
    s32 newflag;
    u32 dir;
    u32 col;

    if (flags & 0x60000) {
        actor_resolve_facing(entity);
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 3;
        group = 1;
        newflag = 0x1000;
        if (R16A(0x06096E90 + M8(entity, ACTOR_SLOT_ID) * 2) != 0)
            actor_reset_pose_history(entity);
    } else {
        if (flags & 0x30)
            actor_resolve_facing(entity);
        else if (flags & 0x400)
            actor_reset_pose_history(entity);
        M32(entity, 0x88) = 0;
        M8(entity, 0x43) = 4;
        group = 3;
        newflag = 0x2000;
        actor_compute_motion_components(entity);
        M32(entity, 0xC8) += 0x800;
        M32(entity, 0x78) = ((u16)(M16(entity, 0x62)) >= 0x8000) ? 0x3000 : -0x3000;
        M32(entity, 0x80) = -0x3000;
    }

    M32(entity, 0x84) = newflag;

    col = (u16)(M16(entity, 0x60) + 0x4000) >> 14;
    dir = (u16)M16(entity, 0x62) >> 13;
    if (M8(entity, 0x44) != 0)
        dir = (dir + 4) & 7;
    M16(entity, ACTOR_ANIM_REQUEST) = anim_tbl(0x060866F8, group, 0, dir, col);

    M8(entity, 0x46) = 0;
    if (MU32(entity, 0x8C) & 0xFFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
    if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
        sound_queue_push(0xA01200FFu);

    R32A(0x06099780 + (M8(entity, ACTOR_SLOT_ID) - 1) * 4) = R32A(DH_FRAME_TICK_ADDR);
}

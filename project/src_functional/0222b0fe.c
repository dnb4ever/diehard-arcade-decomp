/* player_select_grounded_animation [0x0222b0fe]: Grounded motion flags choose directional animation tables and attack boxes. */
#include "dh.h"
/* 0x0222b0fe  actor_anim_select_grounded  (entity=r4)
 *
 * One of a family of six per-frame actor animation selectors packed contiguously
 * (0x0222b0fe / b1e2 / b2a8 / b42c / b630 / b7e8); direct siblings of the already
 * decompiled 0x0222ae40 (actor_facing_anim_select) and 0x0222baf8
 * (actor_jump_anim_select). census merged all six into one row at 0x0222b0fe;
 * each has its own prologue + rts and is a distinct real entry (find_entry).
 *
 * This variant handles the "grounded / low-energy" case. If the actor is in a
 * special motion state (unk34==2, dead unk56==0, or any of the rich motion
 * flags 0x3FF200 in unk84) it delegates to the elaborate selector 0x0222b7e8
 * (tail call). Otherwise: set up the near/far attack boxes from the facing
 * flags, force unk84=0x800 / unk43=2 / unk88=0, raise the combo SFX
 * (0xA01224FF) when combos are enabled (g_060E0DDC), and pick the ground anim
 * from a flat table g_06086730 indexed by (facing-hi ^ unk44). Finally, if the
 * actor just took a hit (unk8C & 0xFFFFFF) spawn the hit reaction fx.
 *
 * Fields: unk34/43/44/45/46 byte, unk56/5C/62 word, unk84/88/8C long.
 * Branches resolved via braf_resolve (bsrf -> 0x227704, 0x223e70, 0x215f6c,
 * 0x23a9d0; bra -> 0x0222b7e8 tail). Cross-checked vs fn.py. fidelity: HIGH. */
#define R16A(a) (*(s16 *)(uintptr_t)(a))
#define R32A(a) (*(s32 *)(uintptr_t)(a))

extern void player_select_motion_animation(void *entity);          /* elaborate sibling selector */
extern void actor_resolve_facing(void *entity);          /* attack-box set A */
extern void actor_reset_pose_history(void *entity);          /* attack-box set B */
extern void actor_commit_effect_handoffs(void *entity, s32 a1);   /* spawn hit-reaction fx */
extern void sound_queue_push(u32 cmd);                /* sound_queue_push */

void player_select_grounded_animation(void *entity) {
    s32 flags;
    s32 idx;

    if (M8(entity, ACTOR_SLOT_ID) == 2 || M16(entity, 0x56) == 0 ||
        (MU32(entity, 0x84) & 0x3FF200)) {
        player_select_motion_animation(entity);
        return;
    }

    flags = M32(entity, 0x84);
    if (flags & 0x30)
        actor_resolve_facing(entity);
    else if (flags & 0x400)
        actor_reset_pose_history(entity);

    M32(entity, 0x88) = 0;
    M32(entity, 0x84) = 0x800;
    M8(entity, 0x43) = 2;

    if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
        sound_queue_push(0xA01224FFu);

    idx = ((u16)M16(entity, 0x62) >> 15) ^ M8(entity, 0x44);
    M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x06086730 + (s16)idx * 2);

    M8(entity, 0x46) = 0;
    if (MU32(entity, 0x8C) & 0xFFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
}

/* player_select_block_animation [0x0222b1e2]: Block variant delegates complex/dead states; otherwise installs box and animation. */
#include "dh.h"
/* 0x0222b1e2  actor_anim_select_block  (entity=r4)
 *
 * Sibling anim selector (see 0x0222b0fe header). Handles the "guard/block"
 * variant. Delegates to the elaborate selector 0x0222b7e8 (tail call) when the
 * actor is dead (unk56==0) or in a rich motion state (unk84 & 0x3FE200).
 * Otherwise sets the attack boxes from the facing flags, forces
 * unk84=0x1000 / unk43=3 / unk88=0, raises combo SFX 0xA01318FF when combos are
 * enabled (g_060E0DDC), and picks the anim from flat table g_06086734 indexed by
 * the signed guard-direction byte unk44. Hit-reaction fx on unk8C & 0xFFFFFF.
 *
 * Fields: unk43/44/45/46 byte, unk56/5C word, unk84/88/8C long.
 * Branches resolved via braf_resolve (bsrf -> 0x227704, 0x223e70, 0x215f6c,
 * 0x23a9d0; bra -> 0x0222b7e8 tail). Cross-checked vs fn.py. fidelity: HIGH. */
#define R16A(a) (*(s16 *)(uintptr_t)(a))
#define R32A(a) (*(s32 *)(uintptr_t)(a))

extern void player_select_motion_animation(void *entity);
extern void actor_resolve_facing(void *entity);
extern void actor_reset_pose_history(void *entity);
extern void actor_commit_effect_handoffs(void *entity, s32 a1);
extern void sound_queue_push(u32 cmd);

void player_select_block_animation(void *entity) {
    s32 flags;

    if (M16(entity, 0x56) == 0 || (MU32(entity, 0x84) & 0x3FE200)) {
        player_select_motion_animation(entity);
        return;
    }

    flags = M32(entity, 0x84);
    if (flags & 0x30)
        actor_resolve_facing(entity);
    else if (flags & 0x400)
        actor_reset_pose_history(entity);

    M32(entity, 0x88) = 0;
    M8(entity, 0x43) = 3;
    M32(entity, 0x84) = 0x1000;

    if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
        sound_queue_push(0xA01318FFu);

    M16(entity, ACTOR_ANIM_REQUEST) = R16A(0x06086734 + (s16)M8(entity, 0x44) * 2);

    M8(entity, 0x46) = 0;
    if (MU32(entity, 0x8C) & 0xFFFFFF) {
        actor_commit_effect_handoffs(entity, 3);
        M8(entity, 0x45) = 0;
    }
}

#include "dh_work.h"
/* actor_load_pose_animation [0x0223a6dc]: Copies pose animation/frame count from the actor pose table and sets the player action category. */
#include "dh.h"
/* 0x0223a6dc (actor anim-pose load) - shared callee. Loads the pose record
 * g_0608CE8C[unk6A] (8-byte stride): unk5C = record word0 (anim id), unk90 = record word4
 * (unsigned, frame count). If unk34 < 3 (a player slot), force state flag unk84 = 0x400000.
 * fidelity: HIGH - unk6A/34 signed byte, unk5C signed word, unk90 long (zero-extended word),
 * unk84 long; unk34<3 is signed (cmp/ge). No computed branches. */
extern s16 g_0608CE8C[];   /* pose records, 8-byte stride */

void actor_load_pose_animation(void *entity) {
    u8 *rec = (u8 *)&g_0608CE8C[ACTOR_M8(entity, saved_primary_effect_type) * 4];
    ACTOR_WORK(entity)->animation_requested = M16(rec, 0);
    ACTOR_M32(entity, action_ticks) = MU16(rec, 4);
    if (ACTOR_WORK(entity)->slot_id < 3)
        ACTOR_WORK(entity)->action_category = 0x400000;
}

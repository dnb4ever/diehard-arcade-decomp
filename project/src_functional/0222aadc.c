#include "dh_work.h"
/* actor_queue_pose_sound [0x0222aadc]: Queues a per-slot sound when actor type/hit timer and global enable gates allow it. */
#include "dh.h"
/* 0x0222aadc (actor hit/footstep sound) - shared callee run by actor_pose_dispatch
 * (0x22adaa). When the actor is type 0x14 (unkBA&0x3F) OR has no hit timer left (unk56==0),
 * and the global sound-enable g_060E0DDC is nonzero, push a sound command: the per-slot sound
 * id g_06087EE4[unk34-1] in the high bits, ORed with 0xA00000FF, into sound_queue_push.
 * fidelity: HIGH - unkBA word, unk56 word, unk34 signed byte, g_06087EE4 signed word, command
 * (id<<8)|0xA00000FF; computed call resolved via braf_rewrite.py (bsrf -> sound_queue_push). */
extern void sound_queue_push(s32 cmd);   /* sound_queue_push */
extern s32  g_sound_effects_enabled_legacy;               /* sound-enable flag */
extern s16  g_06087EE4[];             /* per-slot sound ids */

void actor_queue_pose_sound(void *entity) {
    if (((ACTOR_MU16(entity, type_flags) & DH_ACTOR_TYPE_MASK) == 0x14 || ACTOR_M16(entity, health) == 0)
        && g_sound_effects_enabled_legacy != 0) {
        sound_queue_push((g_06087EE4[ACTOR_WORK(entity)->slot_id - 1] << 8) | DH_SOUND_CUE_BASE);
    }
}

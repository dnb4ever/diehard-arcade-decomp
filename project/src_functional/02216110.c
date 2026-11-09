/* audio_queue_actor_slot_cue [0x02216110]: Queues the actor slot's mapped sound ID when effects are enabled. */
#include "dh.h"
/* 0x02216110 - emit a per-actor sound if sound is enabled. When g_060E0DDC (sound-enable) is set,
 * push (g_060542E8[entity->unk34 - 1] << 8) | 0xA00000FF via sound_queue_push (sound_queue_push);
 * g_060542E8 maps the actor's slot (unk34) to its sound id. Called by the actor state-step family.
 * fidelity: HIGH - enable global long, unk34 byte, id-table halfword, command shifted left 8. */
extern void sound_queue_push(s32 cmd);   /* sound_queue_push */
extern s32  g_sound_effects_enabled_legacy;               /* sound enable */
extern s16  g_060542E8[];             /* per-slot sound id */

void audio_queue_actor_slot_cue(void *entity) {
    if (g_sound_effects_enabled_legacy != 0)
        sound_queue_push((g_060542E8[M8(entity, 0x34) - 1] << 8) | DH_SOUND_CUE_BASE);
}

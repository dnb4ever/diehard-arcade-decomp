/* audio_reemit_latched [0x02216066]: Queues the last latched sound ID in command class A010 when enabled. */
#include "dh.h"
/* 0x02216066 (sound_reemit_latched) - re-issue the last latched sound id. If sound
 * is enabled (g_060E0DDC) pushes command (g_060E0DE0 << 8) | 0xA01000FF via
 * sound_queue_push. Sibling of sound_emit_latch (0x02216018). fidelity: HIGH. */
extern void sound_queue_push(s32 cmd);
extern s32 g_sound_effects_enabled_legacy;   /* sound-enable flag     */
extern s32 g_audio_latched_sound_id;   /* last latched sound id */

void audio_reemit_latched(void) {
    if (g_sound_effects_enabled_legacy != 0)
        sound_queue_push((g_audio_latched_sound_id << 8) | (s32)DH_SOUND_LATCH_BASE);
}

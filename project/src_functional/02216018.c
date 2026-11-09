/* audio_queue_and_latch [0x02216018]: Queues the sound in command class A010 when enabled and always latches its ID. */
#include "dh.h"
/* 0x02216018 - queue a sound command if sound is enabled. When g_060E0DDC (sound-enable) is set,
 * push (id << 8) | 0xA01000FF via sound_queue_push (sound_queue_push). Always latches the last id into
 * g_060E0DE0.
 * fidelity: HIGH - enable/latch globals long; command id shifted left 8. */
extern void sound_queue_push(s32 cmd);   /* sound_queue_push */
extern s32  g_sound_effects_enabled_legacy;               /* sound enable */
extern s32  g_audio_latched_sound_id;

void audio_queue_and_latch(s32 id) {
    if (g_sound_effects_enabled_legacy != 0)
        sound_queue_push((id << 8) | DH_SOUND_LATCH_BASE);
    g_audio_latched_sound_id = id;
}

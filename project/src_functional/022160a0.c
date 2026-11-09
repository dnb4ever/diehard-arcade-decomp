/* audio_queue_actor_cue [0x022160a0]: Selects a type/variant or slot sound ID and queues it when effects are enabled. */
#include "dh.h"
/* 0x022160a0 (actor_sound_cue) - emit an actor-driven sound. If (actor+0xBA & 0x3F)
 * < 0xB: pick index (u8(actor+0x49) * 2) + (s16(actor+0x64) > 0x10 ? 1 : 0) into the
 * halfword table g_060542B8; otherwise use (s8(actor+0x34) - 1) into g_060542E4. When
 * sound is enabled (g_060E0DDC), push (tableValue << 8) | 0xA00000FF via sound_queue_push.
 * fidelity: HIGH - widths and index math verified against the asm. */
extern void sound_queue_push(s32 cmd);
extern s32 g_sound_effects_enabled_legacy;     /* sound-enable flag */
extern s16 g_060542B8[];
extern s16 g_060542E4[];

void audio_queue_actor_cue(void *actor) {
    s32 en = g_sound_effects_enabled_legacy;

    if ((M16(actor, 0xBA) & DH_ACTOR_TYPE_MASK) < 0xB) {
        s32 lo = MU8(actor, 0x49);
        s32 hi = (M16(actor, 0x64) > 0x10) ? 1 : 0;
        if (en != 0)
            sound_queue_push((g_060542B8[(lo * 2) + hi] << 8) | (s32)DH_SOUND_CUE_BASE);
    } else if (en != 0) {
        sound_queue_push((g_060542E4[M8(actor, 0x34) - 1] << 8) | (s32)DH_SOUND_CUE_BASE);
    }
}

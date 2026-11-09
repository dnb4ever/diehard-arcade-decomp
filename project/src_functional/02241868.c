/* actor_sound_emit_fixed [0x02241868]: Existing map corroborated by source: emit fixed sound cmd 0xA01318FF via func_02215f6c, gated on g_060E0DDC */
#include "dh.h"
/* 0x02241868 (actor_sound_emit_fixed) - emit a fixed sound command (0xA01318FF) via
 * the sound queue, gated on the sound-enable flag g_060E0DDC.
 * fidelity: HIGH - direct; bsrf resolved to sound_queue_push (sound queue push). */
extern s32 g_sound_effects_enabled_legacy;
void sound_queue_push(u32 cmd);

void actor_sound_emit_fixed(void) {
    if (g_sound_effects_enabled_legacy != 0)
        sound_queue_push(DH_SOUND_ACTOR_FIXED_CUE_COMMAND);
}

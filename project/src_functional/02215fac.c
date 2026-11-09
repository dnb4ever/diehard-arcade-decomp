/* audio_queue_scene_sound [0x02215fac]: Selects scene sound record, gates queue request and latches sound id. */
#include "dh.h"
/* 0x02215fac (sound_emit_rec) - conditionally emit a record-driven sound. rec =
 * g_06053DBC[g_06098668] (4-byte records). If rec.u0 == 0 and rec.u3 != 0xFF, or
 * else if rec.u2 == 2 and g_0609868F < 0x4C (signed), then: when sound is enabled
 * (g_060E0DDC) push (rec.u3 << 8) | 0xA01000FF via sound_queue_push, and latch
 * g_060E0DE0 = rec.u3. fidelity: HIGH - byte widths (extu.b), signed 0x4C compare. */
extern void sound_queue_push(s32 cmd);
extern s32 g_scene_index;   /* current record index */
extern s32 g_sound_effects_enabled_legacy;   /* sound-enable flag    */
extern s32 g_audio_latched_sound_id;   /* latched sound id     */
extern s8  g_0609868F;
typedef struct { s8 u0; s8 u1; s8 u2; s8 u3; } Rec4;
extern Rec4 g_06053DBC[];

void audio_queue_scene_sound(void) {
    Rec4 *rec = &g_06053DBC[g_scene_index];
    s32 en = g_sound_effects_enabled_legacy;

    if (rec->u0 == 0 && (u8)rec->u3 != 0xFF) {
        if (en != 0)
            sound_queue_push(((u8)rec->u3 << 8) | (s32)DH_SOUND_LATCH_BASE);
        g_audio_latched_sound_id = (u8)rec->u3;
        return;
    }
    if ((u8)rec->u2 == 2 && g_0609868F < 0x4C) {
        if (en != 0)
            sound_queue_push(((u8)rec->u3 << 8) | (s32)DH_SOUND_LATCH_BASE);
        g_audio_latched_sound_id = (u8)rec->u3;
    }
}

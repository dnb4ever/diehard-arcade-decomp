/* audio_queue_scene_gated_id [0x0221603e]: Queues the supplied sound ID in command class A003 when the scene flag allows it. */
#include "dh.h"
/* 0x0221603e - queue a sound command gated by a per-mode flag. If the byte at
 * g_06053DBC[g_06098668*4 + 1] is 0, push (id << 8) | 0xA00300FF via sound_queue_push
 * (sound_queue_push). g_06098668 selects a 4-byte record in the table g_06053DBC.
 * fidelity: HIGH - mode index long, record flag byte (@+1), id word shifted left 8. */
extern void sound_queue_push(s32 cmd);   /* sound_queue_push */
extern s32  g_scene_index;               /* mode / fade level index */
extern u8   g_06053DBC[];             /* 4-byte records */

void audio_queue_scene_gated_id(s16 id) {
    if (g_06053DBC[g_scene_index * 4 + 1] == 0)
        sound_queue_push((id << 8) | DH_SOUND_SCENE_CUE_BASE);
}

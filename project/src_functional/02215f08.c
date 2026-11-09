/* audio_queue_indexed_cue [0x02215f08]: Maps an index through the sound ID table and queues command class A000. */
#include "dh_band21.h"
/* 0x02215f08 - push an indexed sound command: (g_060542B0[idx] << 8) | 0xA00000FF via
 * sound_queue_push (sound_queue_push). Table at 0x060542B0 (s16 entries).
 * fidelity: HIGH - leaf; mov.w s16 table, low word 0xA00000FF. */
extern void sound_queue_push(s32 cmd);

void audio_queue_indexed_cue(s32 idx) {
    sound_queue_push((MEM16(0x060542B0 + idx * 2) << 8) | DH_SOUND_CUE_BASE);
}

#include "dh_work.h"
/* 0x02215f24  sound_queue_reset: zero the ring write/read indices and clear the 16-entry ring.
 *   *0x060e0dec = 0; *0x060e0de8 = 0; zero_longs(0x060e0df0, 16)  [tail braf to 0x0221327c].
 * fidelity: faithful - two stores, then braf r2 (r2=0xffffd346 => 0x0221327c = zero_longs),
 *   r5=16 / r4=0x060e0df0 set up for the tail call (in/around the delay slot). */
#include "dh_band21.h"

extern void zero_longs(s32 *dst, s32 count);   /* 0x0221327c */

void sound_queue_reset(void) {
    SOUND_QUEUE_INDEX(DH_SOUND_QUEUE_READ_INDEX_ADDR, write_index) = 0;
    SOUND_QUEUE_INDEX(DH_SOUND_QUEUE_READ_INDEX_ADDR, read_index) = 0;
    zero_longs(SOUND_QUEUE_COMMANDS(DH_SOUND_QUEUE_READ_INDEX_ADDR), DH_SOUND_QUEUE_CAPACITY);
}

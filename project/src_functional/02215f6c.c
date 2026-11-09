#include "dh_work.h"
/* sound_queue_push [0x02215f6c]: Stores one command word and wraps the sixteen-entry ring write index. */
/* 0x02215f6c  sound_queue_push: push `req` into the 16-entry sound ring.
 *   idx = *0x060e0dec;  ring[idx] = req;  *0x060e0dec = (idx + 1) & 15.
 * ring base @0x060e0df0, write index @0x060e0dec.
 * fidelity: faithful - load idx, shll2 (*4) index, store, reload idx, +1, and #15, store (rts delay). Leaf. */
#include "dh_band21.h"

void sound_queue_push(s32 req) {
    s32 *idxp = SOUND_QUEUE_INDEX_PTR(DH_SOUND_QUEUE_READ_INDEX_ADDR, write_index);
    s32 idx = *idxp;
    SOUND_QUEUE_COMMANDS(DH_SOUND_QUEUE_READ_INDEX_ADDR)[idx] = req;
    *idxp = (idx + 1) & DH_SOUND_QUEUE_INDEX_MASK;
}

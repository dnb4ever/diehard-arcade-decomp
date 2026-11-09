#include "dh_work.h"
/* task_cancel_below_depth [0x0220fdc8]: Clears handler words on linked tasks whose unsigned depth exceeds the cutoff. */
#include "dh.h"
/* 0x0220fdc8 - walk the active object-node list (from the anchor node 0x06092244, following
 * ->unk4) and clear ->unk0 (the definition pointer) of every node whose tree depth
 * (unkE, unsigned byte) is greater than `depth`. Prunes definitions deeper than a cutoff.
 * fidelity: HIGH - transcribed from m2c; leaf, no calls. */
void task_cancel_below_depth(u8 depth) {
    void *node = (void *)0x06092244;
    while ((node = TASK_PTR(node, next)) != 0) {
        if ((s32)TASK_U8(node, depth) > (s32)depth)
            TASK_S32(node, handler) = 0;
    }
}

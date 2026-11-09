/* grid_remove_node [0x02217d22]: Unlinks a node through its cached bucket head and next link, then clears its membership flag. */
#include "dh.h"
#include <stdint.h>
/* 0x02217d22 (grid_remove) - unlink an object from its spatial-grid bucket list. The
 * bucket head slot address is cached at obj+0x40; the list is threaded through each
 * node`s +0x44 link. If the head is the object it is popped (*slot = obj->next);
 * otherwise the list is walked until the node whose +0x44 == obj is found and spliced
 * out. Always clears obj+0x0 = 0. fidelity: HIGH - control flow verified against asm. */

void grid_remove_node(void *e) {
    s32 self = (s32)(uintptr_t)e;
    s32 *pp = (s32 *)(uintptr_t)M32(e, 0x40);
    s32 head = *pp;

    if (head != 0) {
        if (head == self) {
            *pp = M32(e, 0x44);
        } else {
            while (M32((void *)(uintptr_t)*pp, 0x44) != 0) {
                s32 cur = *pp;
                if (M32((void *)(uintptr_t)cur, 0x44) == self) {
                    M32((void *)(uintptr_t)cur, 0x44) = M32(e, 0x44);
                    break;
                }
                pp = (s32 *)(uintptr_t)((u32)cur + 0x44);
            }
        }
    }
    M32(e, 0x0) = 0;
}

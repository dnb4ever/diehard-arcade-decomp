/* task_pool_reset [0x0220fc18]: Builds the free chain of 511 0x20-byte task nodes. */
#include "dh.h"
/* 0x0220fc18 - free-list initialiser for the object-node pool. The pool control block
 * lives at 0x06092238 (free head), 0x0609223c / 0x06092240 (active-list anchors) and
 * 0x06092244 (sentinel node, also the "out of nodes" return value). Clears the sentinel's
 * first 3 longs, then builds a singly-linked free list of 511 nodes of 0x20 bytes each,
 * starting at 0x06092264 (= 0x06092244 + 0x20), chaining node->unk4 to the next and
 * terminating the last with NULL; the free head 0x06092238 points at the first node.
 * The real entry is 0x20fc18; the fn.units member 0x20fc16 is a stray trailing pool word
 * (0xffff) from the previous unit. fidelity: HIGH - transcribed from m2c; no branches/calls. */
void task_pool_reset(void) {
    void **head = (void **)DH_TASK_LIST_CONTROL_ADDR;
    u8 *node = (u8 *)0x06092244;
    s32 n = 0x1FF;

    M32(node, 0) = 0;
    M32(node, 4) = 0;
    M32(node, 8) = 0;
    while (n != 0) {
        node += 0x20;
        *head = node;
        n--;
        head = (void **)(node + 4);
    }
    *head = 0;
}

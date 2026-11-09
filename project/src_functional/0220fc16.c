/* task_pool_init_source_alias [0x0220fc16]: Builds 511 free nodes of stride 0x20; source bound to entry 0220fc24 in v1.09. */
/* func_0220fc16 (cart 0x20fc16) - leaf.
 * Initialize the object pool at 0x06092238: zero the sentinel node at
 * 0x06092244 and build a 511-node free list (0x20-byte nodes) linked through
 * each node's [4] field, head stored at pool[0], last node's [4]=NULL.
 * Confidence: HIGH. No calls. Faithful to disasm.
 */
#include "decomp.h"

void task_pool_init_source_alias(void) {
    void **slot = (void **)DH_TASK_LIST_CONTROL_ADDR;
    char  *node = (char *)0x06092244;
    s32 n = 0x1FF;

    FS32(node, 0x0) = 0;
    FS32(node, 0x4) = 0;
    FS32(node, 0x8) = 0;
    while (n != 0) {
        node += 0x20;
        *slot = node;
        n--;
        slot = (void **)(node + 4);
    }
    *slot = 0;
}

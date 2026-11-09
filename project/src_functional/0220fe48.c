/* task_spawn_root_list [0x0220fe48]: Allocates root tasks in definition-list order and links them into the active task list. */
#include "dh.h"
/* 0x0220fe48 - bulk-spawn root object nodes for a NULL-terminated array of definition
 * pointers `defs`. Inlines the node allocator: for each non-NULL def it pops a node from
 * the free list g_06092238, zeroes it (8 longs), appends it to the active list anchored at
 * 0x06092244 (preserving array order), stores the def in node->unk0, and sets node->unk8=0
 * and depth node->unkE=0. Writes the advanced free-list head back when done. No free-list
 * exhaustion check (assumes enough nodes). fidelity: HIGH - transcribed from m2c; the
 * inlined clear loop runs 8 times (matching the standalone allocators). */
void task_spawn_root_list(void **defs) {
    void *node = MP((void *)DH_TASK_LIST_CONTROL_ADDR, 0);     /* free-list head */
    void *anchor = (void *)0x06092244;
    void *def;

    while ((def = *defs++) != 0) {
        void *cur = node;
        s32 *p = (s32 *)cur;
        int i;
        node = MP(node, 4);                     /* advance free list */
        for (i = 8; i != 0; i--)
            *p++ = 0;
        MP(cur, 4) = MP(anchor, 4);             /* append after anchor */
        MP(anchor, 4) = cur;
        MP(cur, 0) = def;
        anchor = cur;
        M32(cur, 8) = 0;
        M8(cur, 0x0E) = 0;
    }
    MP((void *)DH_TASK_LIST_CONTROL_ADDR, 0) = node;
}

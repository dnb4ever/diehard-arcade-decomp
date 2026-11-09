#include "dh_work.h"
/* task_insert_after_parent [0x0220fc82]: Pops a free node and splices it directly after its parent; records parent and depth. */
#include "dh.h"
/* 0x0220fc82 - object-node allocator (variant: splice into parent's child list).
 * Pops a node from the global free list g_06092238; if empty returns the sentinel
 * &g_06092238[3] (0x06092244). Zeroes the node (8 longs), links it at the front of
 * `parent`'s child list (node->unk4 = parent->unk4; parent->unk4 = node), stores the
 * definition record `def` in node->unk0, parent in node->unk14, clears node->unk8,
 * and sets node->unkE = parent->unkE + 1 (tree depth). Returns the node.
 * fidelity: HIGH - transcribed from m2c/asm; no computed branches. The clear loop
 *   runs 8 times (tst/bf.s/add#-1 with count=8), matching list_node_alloc (0x20fd06);
 *   m2c's structural rewrite shows 7 because it hoists the delay-slot decrement. */
#define FREELIST DH_TASK_LIST_CONTROL_ADDR

void *task_insert_after_parent(void *parent, void *def) {
    void *node = MP((void *)FREELIST, 0);
    s32 *p;
    int i;

    if (node == 0)
        return (void *)(FREELIST + 12);

    MP((void *)FREELIST, 0) = TASK_PTR(node, next);      /* pop: head = node->next */
    p = (s32 *)node;
    for (i = 8; i != 0; i--)
        *p++ = 0;                               /* zero first 8 longs */

    TASK_PTR(node, next) = TASK_PTR(parent, next);                /* splice into parent's child list */
    TASK_PTR(parent, next) = node;
    TASK_PTR(node, handler) = def;
    TASK_PTR(node, parent) = parent;
    TASK_S32(node, work) = 0;
    TASK_S8(node, depth) = (s8)(TASK_S8(parent, depth) + 1);
    return node;
}

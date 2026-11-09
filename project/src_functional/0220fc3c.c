#include "dh_work.h"
/* task_insert_after_current [0x0220fc3c]: Pops a free node and splices it after the current-list anchor; records parent and depth. */
#include "dh.h"
/* 0x0220fc3c - object-node allocator (variant: splice into the active list anchored at
 * *g_06092240). Pops a node from the free list g_06092238 (empty -> sentinel 0x06092244),
 * zeroes it (8 longs), splices it after the anchor *(0x06092240)
 * (node->unk4 = anchor->unk4; anchor->unk4 = node; *(0x06092240) = node), stores
 * def (arg1) in node->unk0, parent (arg0) in node->unk14, clears node->unk8, sets
 * node->unkE = arg0->unkE + 1, and returns the node.
 * fidelity: HIGH - transcribed from m2c/asm; no computed branches; clear loop runs 8x. */
#define FREELIST DH_TASK_LIST_CONTROL_ADDR

void *task_insert_after_current(void *parent, void *def) {
    void *node = MP((void *)FREELIST, 0);
    void *anchor;
    s32 *p;
    int i;

    if (node == 0)
        return (void *)(FREELIST + 12);

    MP((void *)FREELIST, 0) = TASK_PTR(node, next);
    p = (s32 *)node;
    for (i = 8; i != 0; i--)
        *p++ = 0;

    anchor = MP((void *)(FREELIST + 8), 0);     /* *(0x06092240) */
    TASK_PTR(node, next) = TASK_PTR(anchor, next);
    TASK_PTR(anchor, next) = node;
    MP((void *)(FREELIST + 8), 0) = node;
    TASK_PTR(node, handler) = def;
    TASK_PTR(node, parent) = parent;
    TASK_S32(node, work) = 0;
    TASK_S8(node, depth) = (s8)(TASK_S8(parent, depth) + 1);
    return node;
}

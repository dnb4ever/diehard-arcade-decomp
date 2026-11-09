/* task_alloc_into_list [0x0220fcc4]: Pops free node, zeroes eight words and splices into caller-supplied list. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220fcc4 - object-node allocator (variant: caller supplies the list to splice into).
 * Pops a node from the free list g_06092238 (empty -> sentinel 0x06092244), zeroes it
 * (8 longs), links it at the front of `list`'s child list (node->unk4 = list->unk4;
 * list->unk4 = node), stores def (arg2) in node->unk0, parent (arg1) in node->unk14,
 * clears node->unk8, sets node->unkE = arg1->unkE + 1, and returns the node.
 * fidelity: HIGH - transcribed from m2c/asm; no computed branches; clear loop runs 8x. */
#define FREELIST DH_TASK_LIST_CONTROL_ADDR

void *task_alloc_into_list(void *list, void *parent, void *def) {
    void *node = MP((void *)FREELIST, 0);
    s32 *p;
    int i;

    if (node == 0)
        return (void *)(FREELIST + 12);

    MP((void *)FREELIST, 0) = TASK_PTR(node, next);
    p = (s32 *)node;
    for (i = 8; i != 0; i--)
        *p++ = 0;

    TASK_PTR(node, next) = TASK_PTR(list, next);
    TASK_PTR(list, next) = node;
    TASK_PTR(node, handler) = def;
    TASK_PTR(node, parent) = parent;
    TASK_S32(node, work) = 0;
    TASK_S8(node, depth) = (s8)(TASK_S8(parent, depth) + 1);
    return node;
}

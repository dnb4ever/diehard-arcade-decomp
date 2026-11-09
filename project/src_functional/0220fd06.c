/* 0x0220fd06  list_node_alloc: pop a node from the free list at g_06092238, zero it, splice
 * it into the active list anchored at g_0609223c, initialise a few fields from the two args
 * and return it (or &g_06092238[3] = base+12 when the free list is empty).
 *   node = *g_06092238;
 *   if (node == NULL) return (u8*)0x06092238 + 12;
 *   *g_06092238 = node->next (@+4);                  (unlink free)
 *   zero node[0..7] (8 longs = 32 bytes);
 *   anchor = *g_0609223c;
 *   node->next(@+4) = anchor->next(@+4);
 *   anchor->next(@+4) = node;  *g_0609223c = node;   (splice active)
 *   node[0] = arg5;  node[20] = arg0;  node[8] = 0;
 *   node[14] = (u8)(arg0[14] + 1);
 *   return node;
 * fidelity: HIGH - self-contained (no calls); the clear loop runs 8 times (tst r13,r13 /
 *   bf.s / add #-1 with r13=8); field offsets/widths match (mov.l links, mov.b +14). arg0
 *   is dereferenced (+14), so it is a pointer; arg5 stored raw into node[0]. */
#include "decomp.h"

void *list_node_alloc(u8 *arg0, s32 arg5) {
    u8 *node = (u8 *)(uintptr_t)MEM32(DH_TASK_LIST_CONTROL_ADDR);
    u8 *anchor;
    s32 *p;
    int i;

    if (node == NULL)
        return (u8 *)(uintptr_t)(DH_TASK_LIST_CONTROL_ADDR + 12);

    MEM32(DH_TASK_LIST_CONTROL_ADDR) = FU32(node, 4);
    p = (s32 *)node;
    for (i = 8; i != 0; i--)
        *p++ = 0;

    anchor = (u8 *)(uintptr_t)MEM32(0x0609223c);
    FU32(node, 4) = FU32(anchor, 4);
    FPTR(anchor, 4) = node;
    MEM32(0x0609223c) = (u32)(uintptr_t)node;

    FS32(node, 0) = arg5;
    FPTR(node, 20) = arg0;
    FS32(node, 8) = 0;
    FU8(node, 14) = (u8)(FU8(arg0, 14) + 1);
    return node;
}

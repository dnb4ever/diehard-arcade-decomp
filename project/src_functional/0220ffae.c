/* task_spawn_list_work6c [0x0220ffae]: Allocates a list node then one zeroed 0x6c-byte work record. */
#include "dh.h"
/* 0x0220ffae - constructor: allocate an object node via list_node_alloc (0x20fd06) with
 * this call's args, then initialise it as a 1-entry pool of 0x6c-byte records
 * (func_02214bb8(node,1,0x6c)); returns the node. fidelity: HIGH - transcribed from asm. */
void *list_node_alloc(void *parent, void *def);        /* 0x0220fd06 */
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_list_work6c(void *a, void *b) {
    void *node = list_node_alloc(a, b);
    task_alloc_zeroed_array(node, 1, 0x6c);
    return node;
}

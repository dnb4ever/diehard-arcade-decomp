/* task_spawn_child_work88 [0x0220feec]: Inserts after parent and allocates one cleared 0x88-byte work record. */
#include "dh.h"
/* 0x0220feec - constructor: allocate an object node via func_0220fc82(a,b) and initialise
 * it as a 1-entry pool of 0x88-byte records (func_02214bb8(node,1,0x88)); returns the node.
 * fidelity: HIGH - transcribed from asm. */
void *task_insert_after_parent(void *parent, void *def);
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_child_work88(void *a, void *b) {
    void *node = task_insert_after_parent(a, b);
    task_alloc_zeroed_array(node, 1, 0x88);
    return node;
}

/* task_spawn_current_work6c [0x0220fece]: Inserts after current and allocates one cleared 0x6c-byte work record. */
#include "dh.h"
/* 0x0220fece - constructor: allocate an object node via func_0220fc3c(a,b) and initialise
 * it as a 1-entry pool of 0x6c-byte records (func_02214bb8(node,1,0x6c)); returns the node.
 * (sibling of func_0220feb0 / func_0220ff72.) fidelity: HIGH - transcribed from asm. */
void *task_insert_after_current(void *parent, void *def);
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_current_work6c(void *a, void *b) {
    void *node = task_insert_after_current(a, b);
    task_alloc_zeroed_array(node, 1, 0x6c);
    return node;
}

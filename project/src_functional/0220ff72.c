/* task_spawn_child_work40 [0x0220ff72]: Inserts after parent and allocates one cleared 0x40-byte work record. */
#include "dh.h"
/* 0x0220ff72 - constructor: allocate an object node via func_0220fc82(a,b) and initialise
 * it as a 1-entry pool of 0x40-byte records (func_02214bb8(node,1,0x40)); returns the node.
 * (sibling of func_0220feb0; shared callee of the sprite-init family 0x20f19e/f202/f26a.)
 * fidelity: HIGH - transcribed from asm. */
void *task_insert_after_parent(void *parent, void *def);
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_child_work40(void *a, void *b) {
    void *node = task_insert_after_parent(a, b);
    task_alloc_zeroed_array(node, 1, 0x40);
    return node;
}

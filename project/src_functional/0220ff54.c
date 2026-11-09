/* task_spawn_current_work40 [0x0220ff54]: Inserts after current and allocates one cleared 0x40-byte work record. */
#include "dh.h"
/* 0x0220ff54 - constructor: allocate an object node via func_0220fc3c(a,b) and initialise
 * it as a 1-entry pool of 0x40-byte records (func_02214bb8(node,1,0x40)); returns node.
 * fidelity: HIGH - transcribed from asm. */
void *task_insert_after_current(void *parent, void *def);
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_current_work40(void *a, void *b) {
    void *node = task_insert_after_current(a, b);
    task_alloc_zeroed_array(node, 1, 0x40);
    return node;
}

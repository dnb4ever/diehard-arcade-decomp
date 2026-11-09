/* task_spawn_actor_work [0x0220ff0a]: Inserts after parent and allocates 0x18c bytes; actor-slot registrar installs this work at task+8. */
#include "dh.h"
/* 0x0220ff0a - constructor: allocate an object node via func_0220fc82(a,b) and initialise
 * it as a 1-entry pool of 0x18c-byte records (func_02214bb8(node,1,0x18c)); returns node.
 * fidelity: HIGH - transcribed from asm. */
void *task_insert_after_parent(void *parent, void *def);
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_actor_work(void *a, void *b) {
    void *node = task_insert_after_parent(a, b);
    task_alloc_zeroed_array(node, 1, 0x18c);
    return node;
}

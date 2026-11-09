/* task_spawn_list_work7c [0x0220ff28]: Inserts into selected list and allocates one cleared 0x7c-byte work record. */
#include "dh.h"
/* 0x0220ff28 - constructor: allocate an object node via func_0220fcc4, splicing it into
 * the list held in *g_060987b8 with this call's args as parent/def, then initialise it as
 * a 1-entry pool of 0x7c-byte records (func_02214bb8(node,1,0x7c)); returns the node.
 * fidelity: HIGH - transcribed from asm (first call: r4 = *(0x060987b8), r5 = arg0,
 *   r6 = arg1; no computed branches). */
void *task_alloc_into_list(void *list, void *parent, void *def);
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);

void *task_spawn_list_work7c(void *parent, void *def) {
    void *node = task_alloc_into_list(MP((void *)0x060987b8, 0), parent, def);
    task_alloc_zeroed_array(node, 1, 0x7c);
    return node;
}

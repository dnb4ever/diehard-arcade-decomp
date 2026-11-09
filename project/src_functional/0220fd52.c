/* task_spawn_numbered_children [0x0220fd52]: Repeats task allocation, writes consecutive child+0d indices and returns count. */
#include "dh.h"
/* 0x0220fd52 - allocate up to `count` object nodes via func_0220fc3c(parent, def), numbering
 * each new node->unkD = 0,1,2,... Stops early if the allocator runs out (returns the
 * sentinel 0x06092244). Returns the number of nodes created (signed byte).
 * fidelity: HIGH - transcribed from m2c; the only call resolves to the 0x20fc3c allocator. */
void *task_insert_after_current(void *parent, void *def);

s8 task_spawn_numbered_children(void *parent, void *def, s32 count) {
    s8 n = 0;
    while (count-- != 0) {
        void *node = task_insert_after_current(parent, def);
        if (node == (void *)0x06092244)
            break;
        M8(node, 0x0D) = n;
        n++;
    }
    return n;
}

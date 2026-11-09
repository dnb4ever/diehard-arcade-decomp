/* task_spawn_definition_list [0x0220fd90]: Allocates one task per nonnull definition until sentinel or list end. */
#include "dh.h"
/* 0x0220fd90 - allocate one object node per non-NULL entry of the pointer array `defs`,
 * each via func_0220fc3c(parent, defs[i]). Stops at the first NULL entry or when the
 * allocator runs out (returns the sentinel 0x06092244). Returns the number created.
 * fidelity: HIGH - transcribed from m2c; the only call resolves to the 0x20fc3c allocator. */
void *task_insert_after_current(void *parent, void *def);

s32 task_spawn_definition_list(void *parent, void **defs) {
    s32 n = 0;
    void *def;
    while ((def = *defs++) != 0 && task_insert_after_current(parent, def) != (void *)0x06092244)
        n++;
    return n;
}

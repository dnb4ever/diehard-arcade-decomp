/* task_spawn_child_work6c [0x0220feb0]: Inserts a child task and allocates one zeroed 0x6c-byte work record. */
/* 0x0220feb0  func_0220feb0: create an object via func_0220fc82(a,b) and initialise it as a
 * 1-entry pool of 0x6c-byte records, returning the object.
 *   p = func_0220fc82(a, b);          (bsrf -> 0x20fc82, already in C; a/b forwarded from r4/r5)
 *   func_02214bb8(p, 1, 0x6c);    (pool/freelist init: count=1, size=0x6c)
 *   return p;
 * Shipped as an extra: this is the clean function body that begins at 0x20feb0; the
 * fn.units member 0x20fe9e is trailing-pool-contaminated and find_entry maps it to the
 * separate entry 0x20fece, so 0x20feb0 is named at its own real address.
 * fidelity: HIGH - cross-checked against m2c; both bsrf targets resolve to functions already
 *   in C; r0 (func_0220fc82's return) is the return value; the fc82 call receives this function's
 *   own incoming r4/r5 unchanged, so the two params are forwarded verbatim. */
#include "decomp.h"

void *task_insert_after_parent(void *arg0, s32 arg1);                  /* 0x0220fc82 */
void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);   /* 0x02214bb8 */

void *task_spawn_child_work6c(void *a, s32 b) {
    void *p = task_insert_after_parent(a, b);
    task_alloc_zeroed_array(p, 1, 0x6c);
    return p;
}

/* 0x0220b0a4  obj_init_mtx_handler: initialise an actor as a 1-entry pool of 0xDC-byte
 * records, register it as the active object, and install its per-frame handler.
 * Extra function (between members 0x20b098 and 0x20b0c6) exposed by the 0x20b098 merge;
 * bounded at 0x20b0c6.
 *   func_02214bb8(actor, 1, 0xdc);         (pool/freelist init: count=1, size=0xdc)
 *   *(void**)0x0609882c = actor;           (register as the active object)
 *   *(void**)actor = &obj_rebuild_mtx;     (handler ptr; mova+add -> 0x0220b098)
 * fidelity: HIGH - cross-checked against m2c; the bsrf delta resolves to the pool-init
 *   helper 0x214bb8 (already in C), and the mova+add function pointer resolves to the
 *   sibling member 0x20b098 (ptr_resolve: anchor 0x20b0bc - 0x24). */
#include "decomp.h"

void *task_alloc_zeroed_array(void *pool, s32 count, s32 size);   /* 0x02214bb8 */
void obj_rebuild_mtx(u8 *actor);                         /* 0x0220b098 */

void obj_init_mtx_handler(u8 *actor) {
    task_alloc_zeroed_array(actor, 1, 0xdc);
    *(void **)(uintptr_t)DH_VIEW_TASK_ADDR_U = actor;
    *(void **)actor = (void *)obj_rebuild_mtx;
}

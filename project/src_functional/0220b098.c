/* 0x0220b098  obj_rebuild_mtx: rebuild the forward/inverse matrices of an actor's
 * sub-object. Member of the 0x20b098 unit; fn.extent over-runs its unconditional braf
 * (the whole body is a single tail-jump), so it is bounded at 0x20b0a4.
 *   mtx_build_fwd_inv(*(void**)(actor+8));   (tail: braf -> 0x214a7a)
 * The SH-2 prologue spills and immediately restores r15 (a dead one-slot frame that m2c
 * also drops); the only effect is the tail call with r4 = *(actor+8).
 * fidelity: HIGH - cross-checked against m2c; the braf delta 0x99d6 resolves to the shared
 *   matrix builder 0x214a7a (already in C); mov.l @(8) is a pointer load. */
#include "decomp.h"

void mtx_build_fwd_inv(u8 *obj);   /* 0x02214a7a */

void obj_rebuild_mtx(u8 *actor) {
    mtx_build_fwd_inv(*(u8 **)(actor + 8));
}

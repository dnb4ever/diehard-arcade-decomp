/* 0x0221f952  obj_advance_rebuild_mtx: advance a sub-object's transform field by 0x4000 and
 * rebuild its forward/inverse matrices. Tiny tail-call wrapper (member):
 *   obj = *(void**)(actor+8);
 *   *(s32*)(obj+8) += 0x4000;            (0x4000 = a 90-deg step on a 16-bit-angle field)
 *   mtx_build_fwd_inv(obj);              (tail: braf -> 0x214a7a)
 * The braf delta 0xffff511a resolves to the shared matrix builder 0x214a7a (already in C);
 * fn.extent over-runs the unconditional braf, so the real body is the 7 instructions above
 * (bounded at 0x21f960 with decomp_bounded).
 * fidelity: HIGH - hand-traced vs bounded SH-2 asm and cross-checked against m2c; both 0x8
 *   loads are mov.l (s32/pointer); the add is a 32-bit add of the literal 0x4000. */
#include "decomp.h"

void mtx_build_fwd_inv(u8 *obj);   /* 0x02214a7a */

void obj_advance_rebuild_mtx(u8 *actor) {
    u8 *obj = *(u8 **)(actor + 8);
    *(s32 *)(obj + 8) += 0x4000;
    mtx_build_fwd_inv(obj);
}

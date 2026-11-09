/* actor_store_part_transform [0x0222ec98]: Publishes the active matrix translation to the actor's sixteen-byte body-part record. */
#include "dh.h"
/* 0x0222ec98 (actor, part) - write one body-part transform descriptor.
 * Calls mtx_apply_translation (func_02213b5c; no args, writes the 3-long result to
 * 0x06098b1c), then copies that translation into the per-part slot at
 * g_06097d58 + (actor->unk34 << 8) + (part << 4).
 * fidelity: HIGH - callee ignores r4/r5; dest base computed before bsrf, part*0x10
 * added in the bsrf delay slot. mov.b @(0x34) is signed. */
#define G32(a) (*(s32 *)(uintptr_t)(a))
extern u8 g_06097d58[];
extern void matrix_transform_translation(void); /* mtx_apply_translation */

void actor_store_part_transform(void *actor, s32 part) {
    u8 *dst = &g_06097d58[(M8(actor, 0x34) << 8) + (part << 4)];
    matrix_transform_translation();
    M32(dst, 0) = G32(DH_TRANSFORMED_VECTOR_ADDR + 0);
    M32(dst, 4) = G32(DH_TRANSFORMED_VECTOR_ADDR + 4);
    M32(dst, 8) = G32(DH_TRANSFORMED_VECTOR_ADDR + 8);
}

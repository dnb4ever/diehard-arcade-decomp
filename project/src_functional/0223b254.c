#include "dh_work.h"
/* actor_load_part_offset [0x0223b254]: Copies the variant-selected three-long part offset into the actor slot's render record. */
#include "dh.h"
/* func_0223b254(entity): copy a 12-byte action record. Source index is remapped
 * through byte table g_060E106C[entity->unk6D]; dest index is entity->unk34.
 * Copies three longs from g_0608CF3C[src*12] to g_060996EC[dst*12].
 * The asm scales first, then sign-extends: 23b26a `exts.w r5,r5` truncates the
 * source byte*12 product to 16 bits, and 23b278 `exts.b r3,r3` truncates the
 * dest byte*12 product to 8 bits (both sign-extended). Out-of-range indices
 * therefore wrap, which the previous version omitted (caused a write at
 * 0x0609956c instead of 0x0609966C for unk34 = -32).
 * fidelity: HIGH. */
extern u8 g_0608CF3C[];
extern u8 g_060E106C[];
extern u8 g_060996EC[];

void actor_load_part_offset(void *ent) {
    s32 si = (s16)((s8)M8(g_060E106C, ACTOR_M8(ent, secondary_effect_type)) * 12);
    s32 di = (s8)((s32)ACTOR_M8(ent, slot_id) * 12);
    u8 *src = &g_0608CF3C[si];
    u8 *dst = &g_060996EC[di];
    M32(dst, 0) = M32(src, 0);
    M32(dst, 4) = M32(src, 4);
    M32(dst, 8) = M32(src, 8);
}

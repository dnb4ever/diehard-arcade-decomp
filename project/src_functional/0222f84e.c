/* effect_init_owner_frame_variant [0x0222f84e]: Same owner-frame seeding family, with variant field and next-handler setup. */
#include "dh.h"
/* 0x0222f84e (leafC) - actor sub-state init. ent = obj->unk8; src = obj->unk14->unk8.
 * Looks up a vector record in the table g_06097d58 indexed by
 * ((s8)src->unk34 << 8) + ((u8)ent->unk5F * 16); copies its xyz into
 * ent->{8,0xC,0x10}; copies src->unk34 and src->unk38; ORs g_0604ebe4[ent->unk34]
 * into ent->unk68; sets ent->unk62 |= 0x80, decrements ent->unk58; finally sets
 * obj->unk0 to the next resume handler at 0x0222f8d0 (mova 0x22f8b0 + 0x20).
 * fidelity: HIGH - field34/5F byte, field62/58 16-bit, the rest 32-bit. Leaf.
 * Repairs: record base symbol g_06097d58 (array, lowercase, as in siblings) and
 * the next-state pointer 0x0222f8d0 (was mistyped 0x022f8d0). */
extern u8  g_06097d58[];      /* base of the per-pose vector record table (HWRAM) */
extern s32 g_actor_slot_masks[];      /* per-unk34 flag table (program rodata) */
extern void effect_owner_frame_continuation(void *task); /* next resume handler (mova 0x22f8b0+0x20) */

void effect_init_owner_frame_variant(void *obj) {
    void *ent = MP(obj, 0x8);
    void *src = MP(MP(obj, 0x14), 0x8);
    s32 off = ((s32)M8(src, 0x34) << 8) + (s32)MU8(ent, 0x5F) * 16;
    void *rec = &g_06097d58[off];

    M32(ent, 0x8)  = M32(rec, 0x0);
    M32(ent, 0xC)  = M32(rec, 0x4);
    M32(ent, 0x10) = M32(rec, 0x8);

    M8(ent, 0x34)  = MU8(src, 0x34);
    M32(ent, 0x38) = M32(src, 0x38);

    M32(ent, 0x68) = M32(ent, 0x68)
                   | g_actor_slot_masks[(s32)M8(ent, 0x34)];
    M16(ent, 0x62) = (s16)(M16(ent, 0x62) | 0x80);
    M16(ent, 0x58) = (s16)(M16(ent, 0x58) - 1);

    MP(obj, 0x0) = (void *)effect_owner_frame_continuation;
}

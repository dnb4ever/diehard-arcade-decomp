#include "dh_work.h"
/* actor_register_movement_slot [0x0221707c]: Stores actor and four caller parameters in movement/collision records consumed by 022170ea. */
#include "dh.h"
/* 0x0221707c - register an actor into the slot-indexed record table g_060B6D9C (0x48 stride). If the
 * actor's slot (entity->unk34) is < 0x20, fill its record: unk4 = slot, unk18 = a1, unk1C = a2,
 * unk20 = a3, unk24 = a4 (5th arg, passed on stack), unk8 = entity pointer.
 * fidelity: HIGH - slot byte (signed), record fields long, unk8 pointer. a4 is the stack-passed
 * 5th argument (@(8,r15)). */
extern u8 g_actor_movement_records[];   /* per-slot records, 0x48 stride */

void actor_register_movement_slot(void *entity, s32 a1, s32 a2, s32 a3, s32 a4) {
    s32 slot = ACTOR_WORK(entity)->slot_id;
    void *rec;

    if (slot < 0x20) {
        rec = g_actor_movement_records + slot * 0x48;
        M32(rec, 0x4)  = slot;
        M32(rec, 0x18) = a1;
        M32(rec, 0x1C) = a2;
        M32(rec, 0x20) = a3;
        M32(rec, 0x24) = a4;
        MP(rec, 0x8)   = entity;
    }
}

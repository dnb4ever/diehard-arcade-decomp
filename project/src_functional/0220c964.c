#include "dh_work.h"
#include "dh.h"
/* 0x0220c964 (actor_pick_spawn_near_object) - scan the 8 object slots, starting at a
 * rotating cursor (self->unkB0), for an active object that sits inside the current room
 * record's XZ bounds and whose immediate surroundings are clear; if found, seat self next
 * to it and return 1, else return 0. self = task->unk8.
 *
 *   rec = &g_0606080C + g_060B3D54 * 44;              // current room/area record
 *   for (i = 0; i < 8; i++) {
 *       slot = (self->unkB0 + i) & 7;
 *       if (g_06099590[slot] == NULL) continue;        // slot empty
 *       obj = g_060995B0[slot]->unk8;                  // object; position vec at obj+8
 *       px = obj->unk8; py = obj->unkC; pz = obj->unk10;
 *       if (pz > rec->unk28 - 0x18000) continue;        // outside Z band
 *       if (px < rec->unk14 + 0x20000) continue;        // outside X band (low)
 *       if (px > rec->unk20 - 0x20000) continue;        // outside X band (high)
 *       self->unk90 = px - 0x20000;  self->unk98 = pz - 0x40000;  // trial rect min
 *       self->unk9C = px + 0x20000;  self->unkA4 = pz;            // trial rect max
 *       count_objects_in_rect(&self->unk90, &self->unk9C, &count, &last);
 *       if (count == 0) break;                          // surroundings clear -> take it
 *   }
 *   if (i >= 8) return 0;                                // nothing suitable
 *   self->unkB0 = slot;                                  // advance cursor to found slot
 *   self->unk8  = (px >= self->unk84) ? rec->unk14 + 0x28000 : rec->unk20 - 0x28000;
 *   self->unkC  = py + 0x28000;  self->unk10 = pz;
 *   self->unk24 = self->unk26 = self->unk28 = 0;
 *   return 1;
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (0x20c964..0x20ca6c). Sole bsrf resolves
 * to count_objects_in_rect (0x20e000). g_06099590 (slot flags) = &g_06099584+0xC;
 * g_060995B0 (object ptrs) = &g_06099584+0x2C; both match the sibling 0x20e000. Room
 * record stride is 44 (0x2C). unkB0/unk24/26/28 are halfwords; coords are 16.16. */

extern s8  g_actor_slot_registry;   /* +0xC: slot-flag ptr array[8]; +0x2C: object ptr array[8] */
extern s32 g_060B3D54;   /* current room/area index */
extern s8  g_0606080C;   /* room-record table, stride 44 */

extern void count_objects_in_rect(void *pmin, void *pmax, s32 *count, s32 *last); /* 0x20e000 */

s32 actor_pick_spawn_near_object(void *task) {
    void *self = TASK_PTR(task, work);
    char *rec  = (char *)&g_0606080C + g_060B3D54 * 44;
    s32   i, slot = 0;
    void *obj = (void *)0;
    s32   px = 0, py = 0, pz = 0;
    s32   count, last;

    for (i = 0; i < 8; i++) {
        slot = (M16(self, 0xB0) + i) & 7;
        if (MP(&g_actor_slot_registry, 0xC + slot * 4) == (void *)0)
            continue;

        obj = MP(MP(&g_actor_slot_registry, 0x2C + slot * 4), 0x8);
        px = M32(obj, 0x8);
        py = M32(obj, 0xC);
        pz = M32(obj, 0x10);

        if (pz > M32(rec, 0x28) - 0x18000) continue;
        if (px < M32(rec, 0x14) + 0x20000) continue;
        if (px > M32(rec, 0x20) - 0x20000) continue;

        M32(self, 0x90) = px - 0x20000;
        M32(self, 0x98) = pz - 0x40000;
        M32(self, 0x9C) = px + 0x20000;
        M32(self, 0xA4) = pz;
        count_objects_in_rect(&M32(self, 0x90), &M32(self, 0x9C), &count, &last);
        if (count == 0)
            break;
    }

    if (i >= 8)
        return 0;

    M16(self, 0xB0) = (s16)slot;
    M32(self, 0x8)  = (px >= M32(self, 0x84)) ? M32(rec, 0x14) + 0x28000
                                              : M32(rec, 0x20) - 0x28000;
    M32(self, 0xC)  = py + 0x28000;
    M32(self, 0x10) = pz;
    M16(self, 0x24) = 0;
    M16(self, 0x26) = 0;
    M16(self, 0x28) = 0;
    return 1;
}

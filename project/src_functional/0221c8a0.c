/* event_spawn_matching_frame [0x0221c8a0]: Four-entry selector table matches active frame; seeds child position and marker 0x2ed. */
/* 0x0221c8a0 - conditional one-shot spawner. Reads the active scene/record pointer at
 * g_060995BC, follows ->unk8 to its entity, and compares the entity's (u16) field at +0x58
 * against a 4-entry lookup byte table at 0x0609096C indexed by two bits of the global state
 * word g_0609856C: idx = ((g_0609856C & 4) >> 1) + (g_0609856C & 1). If they match, spawn a
 * child object (spawn_obj_a, parent = this task, handler = func_0221c8f2), publish marker
 * 0x2ED to g_060962B6, and seed the spawned entity's position vector (child->unk8->unk8/0C/10)
 * from the global vec3 at 0x060980A8.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. (flags&4)>>1 is arithmetic shar; the +0x58
 *   field is zero-extended (extu.w) and the table byte zero-extended (extu.b) before compare.
 *   spawn_obj_a 0x20feec; handler ptr 0x21c8f2 via mova(0x21c8cc)+0x26 (ptr_resolve). r4
 *   (parent) is this function's incoming task, passed through to spawn_obj_a. */
#include "dh_band21.h"

extern void *task_spawn_child_work88(void *parent, void *handler); /* spawn_obj_a (returns obj) */
extern void  debris_ballistic_update(void);                        /* spawned child handler */

void event_spawn_matching_frame(void *parent)
{
    u32 rec = MEMU32(0x060995bc);
    u32 ent = MEMU32(rec + 8);
    s32 flags = MEM32(0x0609856c);
    s32 idx = ((flags & 4) >> 1) + (flags & 1);

    if ((u16)MEMU16(ent + 0x58) == (u8)MEM8(0x0609096c + idx)) {
        u32 obj = (u32)(uintptr_t)task_spawn_child_work88(parent, (void *)debris_ballistic_update);
        u32 e2 = MEMU32(obj + 8);
        MEM16(0x060962b6) = 0x2ed;
        MEM32(e2 + 8)  = MEM32(0x060980a8);
        MEM32(e2 + 12) = MEM32(0x060980ac);
        MEM32(e2 + 16) = MEM32(0x060980b0);
    }
}

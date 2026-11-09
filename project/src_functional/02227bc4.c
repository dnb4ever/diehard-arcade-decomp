/* actor_spawn_fixed_slot [0x02227bc4]: Rejects occupied fixed slot, creates 0x18c work and publishes slot id/task pointer. */
#include "dh.h"
/* 0x02227bc4  register a spawned object at a fixed id (arg0 + 1) in the entity-slot table at
 * 0x06099584. Fails (NULL) if that id slot (id[] at +0xC) is already taken. Otherwise spawns via
 * func_0220ff0a(context 0x06099580), records id and pointer (ptr[] at +0x2C), clears the per-slot
 * 16-bit state word at 0x06099790[arg0], and stamps the id into the object's sub-struct
 * (*(obj+8) + 0x34). Fixed-id companion to func_02227b60.
 * fidelity: HIGH - matches asm. */
extern u8  g_actor_slot_registry[];     /* entity table: +0xC id[], +0x2C ptr[] */
extern s32 g_06099580;       /* spawn context */
extern u16 g_06099790[];     /* per-slot state words */
extern void *task_spawn_actor_work(s32 context);
void *actor_spawn_fixed_slot(s32 arg0) {
    s32 id = arg0 + 1;
    void *obj;
    if (M32(g_actor_slot_registry, 0xC + id * 4) != 0) {
        return (void *)0;
    }
    obj = task_spawn_actor_work(g_06099580);
    M32(g_actor_slot_registry, 0xC + id * 4) = id;
    MP(g_actor_slot_registry, 0x2C + id * 4) = obj;
    g_06099790[arg0] = 0;
    M8(MP(obj, 8), 0x34) = (s8)id;
    return obj;
}

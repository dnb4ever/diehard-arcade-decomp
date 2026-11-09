/* actor_spawn_dynamic_slot [0x02227b60]: Allocates actor work in a free dynamic slot and publishes its task, ID and live count. */
#include "dh.h"
/* 0x02227b60  register a newly spawned object into the dynamic entity-slot table at 0x06099584
 * (slots 3..). Fails (NULL) once the live count (+8) reaches 5. Finds the first free id slot
 * (id[] at +0xC), spawns the object via func_0220ff0a(context 0x06099580, arg0), bumps the count,
 * records the id and object pointer (ptr[] at +0x2C), and stamps the slot id into the object's
 * sub-struct (*(obj+8) + 0x34). Companion to func_02227bc4 (fixed-id variant).
 * fidelity: HIGH - matches asm. */
extern u8  g_actor_slot_registry[];     /* entity table: +8 count, +0xC id[], +0x2C ptr[] */
extern s32 g_06099580;       /* spawn context */
extern void *task_spawn_actor_work(s32 context, s32 arg);
void *actor_spawn_dynamic_slot(s32 arg0) {
    s32 id;
    void *obj;
    if (M32(g_actor_slot_registry, 8) >= 5) {
        return (void *)0;
    }
    id = 2;
    do {
        id++;
    } while (M32(g_actor_slot_registry, 0xC + id * 4) != 0);
    obj = task_spawn_actor_work(g_06099580, arg0);
    M32(g_actor_slot_registry, 8) += 1;
    M32(g_actor_slot_registry, 0xC + id * 4) = id;
    MP(g_actor_slot_registry, 0x2C + id * 4) = obj;
    M8(MP(obj, 8), 0x34) = (s8)id;
    return obj;
}

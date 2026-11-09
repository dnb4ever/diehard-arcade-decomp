/* actor_system_construct [0x02227790]: Allocates 0x6c work, publishes owner, clears eight slots and installs runtime handler. */
#include "dh.h"
/* 0x02227790  construct the dynamic entity system. Allocates/initialises the owner object via
 * func_02214bb8(self, 1, 0x6C), publishes it to the global context pointer 0x06099580, installs its
 * per-frame handler (runtime 0x0602B830, built via mova 0x0602B7A8 + 0x88) as self's vtable (+0),
 * clears the 8 id/pointer slots of the table at 0x06099584, runs the full reset func_02227a84, and
 * tail-calls func_0223c95c.
 * fidelity: HIGH - handler literal is the runtime (HWRAM) address 0x0602B830 (symbol 0x02227830
 * offset by the 0x06004000 - 0x02200000 mapping), matching the ROM's mova/add computation. */
extern void *g_06099580;     /* global entity-system context pointer */
extern u8   g_actor_slot_registry[];    /* entity table: +0xC id[], +0x2C ptr[] */
extern void task_alloc_zeroed_array(void *self, s32 count, s32 size);
extern void actor_reset_slot_registry(void);
extern void effect_pool_reset_scene(void);
void actor_system_construct(void *self) {
    s32 i;
    task_alloc_zeroed_array(self, 1, 0x6C);
    g_06099580 = self;
    MP(self, 0) = (void *)(uintptr_t)0x0602B830;
    for (i = 0; i < 8; i++) {
        M32(g_actor_slot_registry, 0xC + i * 4) = 0;
        MP(g_actor_slot_registry, 0x2C + i * 4) = (void *)0;
    }
    actor_reset_slot_registry();
    effect_pool_reset_scene();
}

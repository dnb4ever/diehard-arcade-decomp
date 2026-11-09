/* player_get_active_work [0x022461d8]: Slots 1..2 require active flag then return task work pointer. */
#include "dh.h"
/* 0x022461d8  look up player/slot object by index (1..2) in g_06099584 and
 * return its field[8]. Returns 0 if idx out of range or the slot's enable
 * flag (at +12+idx*4) is 0.
 * fidelity: HIGH - exact (cmp/ge, cmp/gt guard 1<=idx<=2). */
extern u8 g_actor_slot_registry[];   /* slot table (HWRAM) */
s32 player_get_active_work(s32 idx) {
    if (idx < 1 || idx > 2)
        return 0;
    if (M32(g_actor_slot_registry, 12 + idx * 4) == 0)
        return 0;
    void *obj = MP(g_actor_slot_registry, 44 + idx * 4);
    return M32(obj, 8);
}

/* actor_allocate_effect_slot [0x02228106]: Scans byte occupancy table, marks a free entry, returns index+8 or -1. */
#include "dh.h"
/* 0x02228106  find first free byte slot in 0x060996D4[0..0x17]; mark it 1; return idx+8, or -1 if full.
 * fidelity: HIGH - exact. */
extern u8 g_effect_slot_occupied[];
s32 actor_allocate_effect_slot(void) {
    s32 i = -1;
    do {
        i += 1;
    } while (g_effect_slot_occupied[i] != 0);
    if (i >= 0x18) {
        return -1;
    }
    g_effect_slot_occupied[i] = 1;
    return i + 8;
}

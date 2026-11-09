/* rescale_active_slots [0x0222ea94]: Existing map corroborated by source: void(s32 level). if(level!=g_06096C90 && level>=0): for 8 slots with flag set, M8((*(ptr+8))+0x40) = ashl_var(that_byte, level-g_06096C90); then g_06096C90=level. 4 ashl_var (4x unrolled, 2 outer passes). Hand-verified. */
#include "dh_work.h"
#include "dh.h"
/* 0x0022ea94  rescale_active_slots  (shift each active slot's byte field by the
 * change in the global scale level).
 *
 * Previously deferred: four calls to the 0x24a360 shift-left helper (ashl_var)
 * via the non-standard r0/r1 bsrf ABI (value in r0 delay slot, count in r1).
 * braf_resolve.py: all four bsrf resolve to 0x24a360. The four are a 4x unrolled
 * body of a loop that runs 8 slots total (counter += 4 per pass, stops at 8).
 *
 * If arg0 differs from the current level g_06096C90 and is non-negative, walk
 * the eight slots; for each slot whose flag g_flags[i] is set, left-shift the
 * signed byte at offset 0x40 of that slot's record (*(slot_ptr+8)) by
 * (arg0 - g_06096C90), then record the new level in g_06096C90.
 *
 * The shift count (arg0 - old level) can be negative or >=32, so the helper's
 * edge cases matter; func_0224a360 is called rather than inlined as `<<`.
 *
 * fidelity: HIGH - the guard, 8-slot loop, flag gate, double pointer indirection
 * and the signed-byte load/shift/store all read from ROM asm; shift target
 * confirmed by braf_resolve.py (-> 0x24a360). */
extern s32   g_06096C90;        /* current applied scale level */
extern s32   g_actor_slot_ids[8];     /* per-slot active flags */
extern void *g_actor_slot_tasks[8];     /* per-slot record pointers */
extern s32 shc_shift_left(s32 value, s32 count);   /* ashl_var: value << count */

void rescale_active_slots(s32 level) {
    s32 delta;
    int i;

    if (level == g_06096C90 || level < 0)
        return;

    delta = level - g_06096C90;
    for (i = 0; i < 8; i++) {
        if (g_actor_slot_ids[i] != 0) {
            void *rec = TASK_PTR(g_actor_slot_tasks[i], work);
            M8(rec, 0x40) = (s8)shc_shift_left(M8(rec, 0x40), delta);
        }
    }
    g_06096C90 = level;
}

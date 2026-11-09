#include "dh_work.h"
/* actor_debounce_frame_event [0x02241556]: Counts events within sixty frame ticks and compares the count with the requested limit. */
#include "dh.h"
/* 0x02241556  per-struct frame debounce/timer against a global frame counter.
 * diff = *g_060987bc - field[0x110]; if diff > 60 (unsigned): reset counter
 * field[0x10c]=1, stamp field[0x110], return 1. Otherwise bump field[0x10c];
 * if it reached arg1 return 0, else re-stamp and return 1.
 * fidelity: HIGH - exact (cmp/hi = unsigned >, cmp/ge = signed >=). */
extern u32 g_frame_tick_legacy;      /* global frame counter (read-only here) */
s32 actor_debounce_frame_event(void *p, s32 arg1) {
    u32 cur = g_frame_tick_legacy;
    u32 diff = cur - ACTOR_MU32(p, event_tick);
    if (diff > 60) {
        ACTOR_M32(p, event_count) = 1;
        ACTOR_M32(p, event_tick) = (s32)cur;
        return 1;
    }
    s32 c = ACTOR_M32(p, event_count) + 1;
    ACTOR_M32(p, event_count) = c;
    if (c >= arg1)
        return 0;
    ACTOR_M32(p, event_tick) = (s32)cur;
    return 1;
}

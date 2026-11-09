#include "dh_work.h"
/* actor_debounce_long_event [0x0224158c]: Counts events within ninety frame ticks and compares with the supplied limit. */
#include "dh.h"
/* 0x0224158c (leafC) - retrigger/cooldown helper on an object. t = g_060987BC (a frame
 * timer). If (u32)(t - obj->field110) > 90, the stamp has expired: reset counter
 * obj->field10C = 1, restamp field110 = t, return 1. Otherwise bump the counter; when it
 * reaches arg1 return 0 (done), else restamp field110 = t and return 1 (still active).
 * fidelity: HIGH - 32-bit fields; the window compare is unsigned, the count compare signed. */
extern s32 g_frame_tick;
s32 actor_debounce_long_event(void *obj, s32 limit) {
    s32 t = g_frame_tick;
    if ((u32)(t - ACTOR_M32(obj, event_tick)) > 90) {
        ACTOR_M32(obj, event_count) = 1;
        ACTOR_M32(obj, event_tick) = t;
        return 1;
    }
    {
        s32 c = ACTOR_M32(obj, event_count) + 1;
        ACTOR_M32(obj, event_count) = c;
        if (c >= limit)
            return 0;
        ACTOR_M32(obj, event_tick) = t;
        return 1;
    }
}

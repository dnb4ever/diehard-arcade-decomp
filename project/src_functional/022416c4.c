#include "dh_work.h"
/* actor_clamp_target_heading [0x022416c4]: Clamps heading to a target-relative quadrant except for exempt actor types. */
#include "dh.h"
/* 0x022416c4 (actor_facing_clamp) - clamp an actor's facing angle (+0x62) into a
 * quadrant depending on whether it is closing on or past its target distance.
 * Real entry recovered by find_entry (member 0x2416ac is pool/data-contaminated;
 * this is the real function start -> counted as an extra).
 *   fl = (a+0xBA) & 0x3F; if 5<=fl<=6 do nothing.
 *   if 1<=state(a+0x4A)<=2: a+0x138 = g_060995B0[state]->+8 ->+8
 *   ua = (u16)angle(a+0x62);
 *   if (a+8 < a+0x138):   near: <0x2000||>=0xC000 ->0x2000; else >0x6000 ->0x6000
 *   else:                 far:  <=0x4000||>0xE000 ->-0x2000; else <0xA000 ->-0x6000
 * fidelity: HIGH - unsigned-u16 angle compares (extu.w), signed +8/+0x138 compares. */
extern void *g_actor_slot_tasks[];

void actor_clamp_target_heading(void *a) {
    s16 angle = ACTOR_M16(a, motion_heading);
    s8  state = ACTOR_M8(a, hit_source_slot);
    s32 fl = ACTOR_M16(a, type_flags) & DH_ACTOR_TYPE_MASK;
    u16 ua;
    s16 out;

    if (fl >= 5 && fl <= 6)
        return;

    if (state >= 1 && state <= 2) {
        void *obj = g_actor_slot_tasks[state];
        M32(a, 0x138) = M32(MP(obj, 8), 8);
    }

    ua = (u16)angle;
    out = angle;
    if (ACTOR_M32(a, position_x) < M32(a, 0x138)) {
        if (ua < 0x2000 || ua >= 0xC000)
            out = 0x2000;
        else if (ua > 0x6000)
            out = 0x6000;
    } else {
        if (ua <= 0x4000 || ua > 0xE000)
            out = (s16)0xE000;   /* -0x2000 */
        else if (ua < 0xA000)
            out = (s16)0xA000;   /* -0x6000 */
    }
    ACTOR_M16(a, motion_heading) = out;
}

/* 0x022416ac  actor facing-angle clamp by state/distance. MISSEG: data table prefixes member; cannot byte-match. Logic functional. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

/* NOTE: fn.py misseg - a data table precedes the real function start; the real
   logic (angle clamp for actor facing) begins partway into the member. */
void f_2416ac(Actor *a, u32 arg1, s32 arg2, s32 arg3) {
    s16 ang;
    s8 state;
    s32 fl;
    ang = a->f62;
    state = a->f4a;
    fl = a->fba & 0x3F;
    if (fl < 5 || fl > 6) {
        if (state >= 1 && state <= 2) {
            a->f138 = (s32)((E **)0x060995B0)[state]->f8->f8;
        }
        if (a->f08 < a->f138) {
            if ((s32)(u16)ang < 0x2000 || (s32)(u16)ang >= 0xC000) {
                ang = 0x2000;
            } else if ((s32)(u16)ang > 0x6000 && (s32)(u16)ang < 0xC000) {
                ang = 0x6000;
            }
        } else if ((s32)(u16)ang <= 0x4000 || (s32)(u16)ang > 0xE000) {
            ang = -0x2000;
        } else if ((s32)(u16)ang > 0x4000 && (s32)(u16)ang < 0xA000) {
            ang = -0x6000;
        }
        a->f62 = ang;
    }
}

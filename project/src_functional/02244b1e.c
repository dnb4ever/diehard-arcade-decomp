/* 0x02244b1e  actor_script_step6: advance one step of an actor's 6-byte-record command
 * script. Internal processor of the 0x244988 fn-unit merge (split with decomp_bounded,
 * end 0x244c4e). Stride-6 sibling of actor_script_step (0x2449b2): the gate word is
 * w = rec[1] (the halfword at rec+2 of a 6-byte record cfg[idx]), with the same
 * bit 0x8000 (terminate) / bit 0x4000 (timed-wait) gates, plus an extra bit 0x2000
 * branch that consults the global dispatch table g_0608db3c (8-byte entries:
 * key@0, threshold@2, callback@4):
 *   bit 0x2000 set, not armed: entry = &g_0608db3c[w & 0x1fff];
 *       if (entry.key == actor+0x58 && actor+0x40 == 0) { entry.callback(actor); return -1; }
 *       if (entry.threshold >= actor+0x58) return -1;  else accept
 *   bit 0x2000 clear, not armed: gate (w & 0xffff1fff) < actor+0x58, else return -1.
 * On accept the command index actor+0x11c is bumped and the NEXT record is handed to
 * actor_script_load (0x244abc), which performs the richer per-record load; returns 1.
 *   return 1 = advanced, 0 = terminated (bit15), -1 = waiting/blocked/callback-fired.
 * fidelity: HIGH - hand-traced vs bounded SH-2 asm and cross-checked against m2c. The
 *   callback is `jsr @Rn` with a nop delay slot and r4 unchanged, so it is called with
 *   the actor as its (only register-live) argument. Widths: mov.l 0x118/0x84/table+4
 *   (s32/ptr); mov.w 0x11c/0x11e/0x58/rec words/table key+thr (s16); 0x54 u16; mov.b 0x40. */
#include "decomp.h"

void actor_script_load(u8 *actor, s16 *cfg);   /* 0x02244abc */

#define SCRIPT_DISPATCH 0x0608db3cu

s32 actor_script_step6(u8 *actor) {
    s16 *cfg = *(s16 **)(actor + 0x118);
    u16 idx = *(u16 *)(actor + 0x11c);
    s16 w = *(s16 *)((char *)cfg + idx * 6 + 2);
    s32 masked = (u16)w & 0x1fff;
    u16 armed = *(u16 *)(actor + 0x54) & 2;
    u16 limit = *(u16 *)(actor + 0x58);
    u16 ni;
    s16 cnt;

    if ((u16)w & 0x8000) {
        if (armed == 0) {
            if (masked == 0)
                return -1;
            if (masked >= (s32)limit)
                return -1;
        }
        *(s32 *)(actor + 0x84) &= ~0x200;
        return 0;
    }

    if ((u16)w & 0x4000) {
        if (armed == 0)
            return -1;
        if (masked == 0)
            return -1;
        cnt = *(s16 *)(actor + 0x11e) + 1;
        *(s16 *)(actor + 0x11e) = cnt;
        if (masked > (u16)cnt)
            return -1;
    } else {
        s32 w13 = (u16)w & 0xffff1fff;
        if ((u16)w & 0x2000) {
            if (armed == 0) {
                s16 *entry = (s16 *)(uintptr_t)(SCRIPT_DISPATCH + w13 * 8);
                if ((u16)limit == entry[0] && *(u8 *)(actor + 0x40) == 0) {
                    void (*cb)(u8 *) = *(void (**)(u8 *))((char *)entry + 4);
                    cb(actor);
                    return -1;
                }
                if (entry[1] >= (s32)limit)
                    return -1;
            }
        } else if (armed == 0 && w13 >= (s32)limit) {
            return -1;
        }
    }

    *(s16 *)(actor + 0x11c) += 1;
    ni = *(u16 *)(actor + 0x11c);
    actor_script_load(actor, (s16 *)((char *)cfg + ni * 6));
    return 1;
}

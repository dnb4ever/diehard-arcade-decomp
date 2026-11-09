/* 0x022449b2  actor_script_step: advance one step of an actor's control-word script.
 * Second real function of the 0x244988 fn-unit merge (an internal processor inside the
 * over-extended 0x244988 member; split out with decomp_bounded, end 0x244aa8).
 *
 * The script base lives at actor+0x118 (saved by actor_init_from_cfg); actor+0x11c is the
 * current command index into a table of 4-byte records. The current record's control word
 * w = rec[1] carries two gate bits and a 13-bit value (masked = w & 0x1fff):
 *   bit 0x8000 (terminate): clear actor+0x84 bit 0x200 and return 0, but when the +0x54
 *     "armed" bit (0x2) is clear it first gates on masked (0 or >= actor+0x58 -> return -1).
 *   bit 0x4000 (timed): requires the +0x54 bit; bumps the per-command tick actor+0x11e and
 *     waits (return -1) until masked <= tick.
 *   neither: when +0x54 bit clear, gate (w & 0xffff1fff) < actor+0x58, else accept.
 * Accept advances the index, resets the tick, sets actor+0x41=1, loads the NEXT record's
 * first word w2, stores w2 & 0x1fff into actor+0x5c, applies its control flags, returns 1.
 *   return 1  = advanced to next command
 *   return 0  = terminated (bit15 command)
 *   return -1 = still waiting / blocked
 * fidelity: HIGH - hand-traced vs bounded SH-2 asm and cross-checked against m2c; field
 *   widths match (mov.l 0x118/0x84 s32; mov.w 0x11c/0x11e/0x58/0x5c s16; +0x54 u16;
 *   mov.b 0x41 u8). cmp/ge are signed over non-negative operands; the tail bsr passes only
 *   r5 = w2 to the shared helper actor_apply_ctrl_flags (0x244c4e). */
#include "decomp.h"

void actor_apply_ctrl_flags(u8 *actor, s32 flags);   /* 0x02244c4e */

s32 actor_script_step(u8 *actor) {
    s16 *cfg = *(s16 **)(actor + 0x118);
    u16 idx = *(u16 *)(actor + 0x11c);
    s16 w = *(s16 *)((char *)cfg + idx * 4 + 2);
    s32 masked = w & 0x1fff;
    u16 armed = *(u16 *)(actor + 0x54) & 2;
    u16 limit = *(u16 *)(actor + 0x58);
    u16 ni;
    s16 w2, cnt;

    if (w & 0x8000) {
        if (armed == 0) {
            if (masked == 0)
                return -1;
            if (masked >= (s32)limit)
                return -1;
        }
        *(s32 *)(actor + 0x84) &= ~0x200;
        return 0;
    }

    if (w & 0x4000) {
        if (armed == 0)
            return -1;
        if (masked == 0)
            return -1;
        cnt = *(s16 *)(actor + 0x11e) + 1;
        *(s16 *)(actor + 0x11e) = cnt;
        if (masked > (u16)cnt)
            return -1;
    } else if (armed == 0) {
        if (((s32)w & 0xffff1fff) >= (s32)limit)
            return -1;
    }

    *(s16 *)(actor + 0x11c) += 1;
    *(s16 *)(actor + 0x11e) = 0;
    *(actor + 0x41) = 1;
    ni = *(u16 *)(actor + 0x11c);
    w2 = *(s16 *)((char *)cfg + ni * 4);
    *(s16 *)(actor + 0x5c) = w2 & 0x1fff;
    actor_apply_ctrl_flags(actor, w2);
    return 1;
}

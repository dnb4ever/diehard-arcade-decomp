/* 0x02244abc  actor_script_load: load the current command record `cfg` into an actor.
 * Internal processor of the 0x244988 fn-unit merge (split with decomp_bounded, end 0x244b1e).
 * Also the fall-through target of the alt-entry actor_script_begin (0x244aa8).
 *
 * Clears the per-command tick (actor+0x11e), copies the record's upper-nibble field
 * cfg[2] & 0xf000 into actor+0x26, marks actor+0x41=1, then dispatches on the record's
 * first word w = cfg[0]:
 *   w == 0xffff : actor+0x5c = actor+0x148      (use the actor's own default value)
 *   w == 0xfffe : actor+0x5c = (**actor+0x158)[cfg[2] & 7]   (indexed table lookup)
 *   otherwise   : actor+0x5c = w & 0x1fff; apply the record's control flags (tail call)
 * fidelity: HIGH - hand-traced vs bounded SH-2 asm and cross-checked against m2c; the two
 *   sentinels fall out of `~(u16)w == 0xffff0000 / 0xffff0001`. Widths: mov.w for
 *   0x11e/0x26/0x148/0x5c/cfg[0]/cfg[2] (s16), mov.b 0x41 (u8), mov.l 0x158 (pointer). */
#include "decomp.h"

void actor_apply_ctrl_flags(u8 *actor, s32 flags);   /* 0x02244c4e */

void actor_script_load(u8 *actor, s16 *cfg) {
    s16 w;

    *(s16 *)(actor + 0x11e) = 0;
    *(s16 *)(actor + 0x26) = cfg[2] & 0xf000;
    *(actor + 0x41) = 1;

    w = cfg[0];
    if ((u16)w == 0xffff) {
        *(s16 *)(actor + 0x5c) = *(s16 *)(actor + 0x148);
    } else if ((u16)w == 0xfffe) {
        s16 **p = *(s16 ***)(actor + 0x158);
        s16 *base = *p;
        *(s16 *)(actor + 0x5c) = base[(u16)cfg[2] & 7];
    } else {
        *(s16 *)(actor + 0x5c) = w & 0x1fff;
        actor_apply_ctrl_flags(actor, w);
    }
}

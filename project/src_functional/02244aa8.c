/* 0x02244aa8  actor_script_begin: (re)start an actor's command script at `cfg`.
 * Alt-entry of the 0x244988 fn-unit merge that falls through into actor_script_load
 * (0x244abc). Resets the command index (actor+0x11c), the per-command tick (actor+0x11e)
 * and the duration accumulator (actor+0x58) to 0, stores the script base pointer at
 * actor+0x118, then loads the first record.
 * fidelity: HIGH - the SH-2 entry does three mov.w zero-stores (0x11c/0x11e/0x58) and one
 *   mov.l store (0x118 = r5) then `bra 0x244abc`; modelled as the stores plus a call into
 *   actor_script_load (which re-zeros 0x11e, matching the original's redundant store). */
#include "decomp.h"

void actor_script_load(u8 *actor, s16 *cfg);   /* 0x02244abc */

void actor_script_begin(u8 *actor, s16 *cfg) {
    *(s16 *)(actor + 0x11c) = 0;
    *(s16 *)(actor + 0x11e) = 0;
    *(s16 *)(actor + 0x58) = 0;
    *(s16 **)(actor + 0x118) = cfg;
    actor_script_load(actor, cfg);
}

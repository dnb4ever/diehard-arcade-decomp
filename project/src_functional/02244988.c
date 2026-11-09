/* 0x02244988  actor_init_from_cfg: initialise an actor from a config record `src`, then
 * apply its control flags. First real entry of the 0x244988 fn-unit merge (the unit also
 * holds several distinct command/state processors - see batch report).
 *   *(void**)(actor+0x118) = src;        (save the config pointer)
 *   *(s16*)(actor+0x11c) = 0; *(s16*)(actor+0x11e) = 0; *(s16*)(actor+0x58) = 0;
 *   *(u8 *)(actor+0x41) = 1;
 *   v = *src;                            (mov.w, s16)
 *   *(s16*)(actor+0x5c) = v & 0x1fff;
 *   actor_apply_ctrl_flags(actor, v);    (tail: bra 0x244c4e, r5 = v)
 * fidelity: HIGH - field widths match (mov.l 0x118 pointer, mov.w 0x11c/0x11e/0x58/0x5c,
 *   mov.b 0x41); the tail bra to the shared helper 0x244c4e passes r5 = the sign-extended
 *   s16 config word (helper re-masks to u16). */
#include "decomp.h"

void actor_apply_ctrl_flags(u8 *actor, s32 flags);   /* 0x02244c4e */

void actor_init_from_cfg(u8 *actor, s16 *src) {
    s16 v;
    *(void **)(actor + 0x118) = src;
    *(s16 *)(actor + 0x11c) = 0;
    *(s16 *)(actor + 0x11e) = 0;
    *(s16 *)(actor + 0x58) = 0;
    *(actor + 0x41) = 1;
    v = *src;
    *(s16 *)(actor + 0x5c) = v & 0x1fff;
    actor_apply_ctrl_flags(actor, v);
}

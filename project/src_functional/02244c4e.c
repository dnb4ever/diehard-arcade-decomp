/* 0x02244c4e  actor_apply_ctrl_flags: apply three control bits from `flags` (low 16 bits)
 * to actor state fields. Shared helper of the 0x244988 merge (bra'd into as a tail by the
 * 0x244988 init and bsr'd from the 0x2449b2 processor); leaf, returns via its own pr.
 *   bit 0x8000 -> actor+0x84 |= 0x200  else clear 0x200 if set
 *   bit 0x4000 -> actor+0xe0 |= 0x08000000 else clear if set
 *   bit 0x2000 -> actor+0x108 = 6  else (if ==6) actor+0x108 = 0
 * fidelity: HIGH (extra - split out of fn-unit member 0x244988). flags masked to u16
 *   (extu.w); each bit's set/clear branch sense verified against the tst/bt/bf chain;
 *   all three touched fields are mov.l (s32). */
#include "decomp.h"

void actor_apply_ctrl_flags(u8 *actor, s32 flags_in) {
    u16 flags = (u16)flags_in;

    if (flags & 0x8000)
        *(s32 *)(actor + 0x84) |= 0x200;
    else if (*(s32 *)(actor + 0x84) & 0x200)
        *(s32 *)(actor + 0x84) &= ~0x200;

    if (flags & 0x4000)
        *(s32 *)(actor + 0xe0) |= 0x08000000;
    else if (*(s32 *)(actor + 0xe0) & 0x08000000)
        *(s32 *)(actor + 0xe0) &= ~0x08000000;

    if (flags & 0x2000)
        *(s32 *)(actor + 0x108) = 6;
    else if (*(s32 *)(actor + 0x108) == 6)
        *(s32 *)(actor + 0x108) = 0;
}

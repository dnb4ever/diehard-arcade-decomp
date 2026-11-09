#include "dh_work.h"
/* actor_latch_action_completion [0x02241b2c]: Restores a saved timing value under its gate and latches the completion flag. */
#include "dh.h"
/* 0x02241b2c  conditional field copy then mask/flag update on field[0xe0].
 * If field[0xe0] bit 0x08000000 is CLEAR (tst sets T=1, bf falls through):
 *     field[0xbc] = field[0x140];
 * Then field[0xe0] &= 0x3640800 (store); field[0xe0] |= 0x400 (store).
 * fidelity: HIGH - branch polarity verified against tst/bf semantics. */
void actor_latch_action_completion(void *p) {
    if (!(ACTOR_M32(p, state_flags) & 0x08000000))
        M32(p, 0xBC) = M32(p, 0x140);
    ACTOR_M32(p, state_flags) &= 0x03640800;
    ACTOR_M32(p, state_flags) |= 0x400;
}

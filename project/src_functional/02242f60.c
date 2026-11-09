/* 0x02242f60  mode_resolve_0x242f60: map the global mode value g_06098564 to a result.
 *   switch (g_06098564) {
 *     case 0: return 0;  case 1: return 1;  case 2: return 2;
 *     case 3: return (rng_next() & 1) + 1;      (-> 1 or 2)
 *     default: return g_06098564;
 *   }
 * fidelity: HIGH - explicit cmp/eq #0..#3 dispatch chain; cases 0/1/2 return their literal
 *   constant (equal to the switch value); case 3 draws rng_next (bsrf delta 0xfffcd08e ->
 *   0x210020) and returns (r0 & 1) + 1; the default path falls through returning the value
 *   still in r0 (the unmatched g_06098564). */
#include "decomp.h"

u32 game_random_next(void);   /* rng_next */

s32 mode_resolve_0x242f60(void) {
    s32 v = *(s32 *)DH_READY_PLAYER_MASK_ADDR;
    switch (v) {
    case 0: return 0;
    case 1: return 1;
    case 2: return 2;
    case 3: return (s32)(game_random_next() & 1) + 1;
    default: return v;
    }
}

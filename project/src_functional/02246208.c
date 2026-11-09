/* actor_flag_rng_1in4 [0x02246208]: Tests the actor's permission flag and a one-in-four random gate. */
/* 0x02246208  actor_flag_rng_1in4: return 1 iff actor+0xe0 bit 0x04000000 is set AND a fresh
 * rng draw has its low two bits clear (a 1-in-4 gate); otherwise 0.
 *   if (!(actor+0xe0 & 0x04000000)) return 0;
 *   if (rng_next() & 3)            return 0;
 *   return 1;
 * fidelity: HIGH - tst/bf flag gate; rng_next via bsrf (delta 0xfffc9e00 -> 0x210020);
 *   tst #3 on the result. Called as actor_flag_rng_1in4 (e.g. from 0x249f52, 0x248436). */
#include "decomp.h"

u32 game_random_next(void);   /* rng_next */

s32 actor_flag_rng_1in4(u8 *actor) {
    if ((*(s32 *)(actor + 0xe0) & 0x04000000) == 0)
        return 0;
    if ((game_random_next() & 3) != 0)
        return 0;
    return 1;
}

/* 0x02241dde  actor_init_rng_dir: (re)init an actor's state and pick a random facing side.
 *   *(s32*)(actor+0xd8) = 2;
 *   *(u8 *)(actor+0x41) = 1;                             (set in rng_next delay slot)
 *   r = rng_next();                                      (func_02210020)
 *   *(s16*)(actor+0x5c) = *(s16*)(0x0608daec + ((r & 1) << 1));
 * fidelity: HIGH - rng_next called via bsrf (delta 0xfffce22c -> 0x210020); the +0x41=1
 *   byte store sits in the delay slot; result masked (r&1), doubled (shll), used as a
 *   halfword index into table g_0608daec; field widths match (mov.l 0xd8, mov.b 0x41,
 *   mov.w 0x5c and the table load). */
#include "dh_band21.h"

u32 game_random_next(void);   /* rng_next */

void actor_init_rng_dir(u8 *actor) {
    u32 r;
    *(s32 *)(actor + 0xd8) = 2;
    *(actor + 0x41) = 1;
    r = game_random_next();
    *(s16 *)(actor + 0x5c) = *(s16 *)(uintptr_t)(0x0608daec + ((r & 1) << 1));
}

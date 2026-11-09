/* 0x02241da8  actor_init_random: (re)initialise an actor's random/turn state.
 *   *(s32*)(actor+0xd8) = 0;
 *   *(s32*)(actor+0xc8) = rng_next() & 15;          (func_02210020, low nibble)
 *   *(s32*)(actor+0xe0) &= 0xffefffff;              (clear bit 20)
 *   *(u8 *)(actor+0x41)  = 1;
 *   *(s16*)(actor+0x5c)  = *(s16*)(actor+0x148);
 * fidelity: HIGH - the "+0xd8 = 0" store sits in the bsrf delay slot (runs with the
 *   call); field offsets loaded as mov.w halves of packed pool longs; widths match
 *   the SH-2 loads/stores (mov.l for 0xc8/0xd8/0xe0, mov.b 0x41, mov.w 0x5c/0x148). */
#include "dh_band21.h"

u32 game_random_next(void);

void actor_init_random(u8 *actor) {
    *(s32 *)(actor + 0xd8) = 0;
    *(s32 *)(actor + 0xc8) = (s32)(game_random_next() & 15);
    *(s32 *)(actor + 0xe0) &= 0xffefffff;
    *(actor + 0x41) = 1;
    *(s16 *)(actor + 0x5c) = *(s16 *)(actor + 0x148);
}

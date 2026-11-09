/* game_random_next [0x02210020]: Updates feedback/multiply-mix PRNG seed and returns the mixed word. */
#include "dh.h"
/* 0x02210020 (rng_next) - the engine PRNG (indeg 133). Seed is g_0609957C.
 * If the low 16 bits of the seed are zero, reseed by XOR 0x2A6D365B. Then:
 *   w = seed*5; w = (w<<3)|((w>>29)&7) [rotl 3]; w += seed;
 *   lo = w & 0xFFFF; hi = w >> 16; result = (((hi+lo)&0xFFFF)<<16) | lo.
 * Store result back to the seed and return it.
 * NOTE: file named at the real entry 0x02210020 (fn.units lumps a preceding data table
 * onto this member 0x20ffd2), so it is an out-of-universe "extra" in the count.
 * fidelity: HIGH - all mov.l; halfword recombine via big-endian @(2) accesses. */
extern u32 g_random_seed;   /* PRNG seed */

u32 game_random_next(void) {
    u32 seed = g_random_seed;
    u32 w, hi, lo;

    if ((seed & 0xFFFF) == 0)
        seed ^= 0x2A6D365B;
    w = seed * 5;
    w = (w << 3) | ((w >> 29) & 7);
    w = w + seed;
    lo = w & 0xFFFF;
    hi = w >> 16;
    w = (((hi + lo) & 0xFFFF) << 16) | lo;
    g_random_seed = w;
    return w;
}

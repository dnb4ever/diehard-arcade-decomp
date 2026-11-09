/* 0x0220ffd2  rng_next (ROM real start 0x2210020). MISSEG: fn.py prepends a data table to this member; whole member cannot byte-match. Logic equivalence-verified. */
/* Status: NONMATCHING (functional). Blocker: whole-unit shared literal-pool + PIC (bsrf/mova) that rof2elf cannot relocate; see unit notes. */
#include "dh_actor.h"

/* rng_next (ROM 0x2210020; fn.py lumps a preceding data table into this member,
   so the member cannot byte-match).  PRNG over seed @0x0609957C. */
u32 f_20ffd2(void) {
    u32 *seed = (u32 *)0x0609957C;
    u32 w = *seed;
    u32 orig;
    u32 hi, lo;
    if ((w & 0xFFFF) == 0) {
        w ^= 0x2A6D365B;
    }
    orig = w;
    w = orig * 5;
    w = (w << 3) | ((w >> 29) & 7);
    w = w + orig;
    hi = w >> 16;
    lo = w & 0xFFFF;
    w = (((hi + lo) & 0xFFFF) << 16) | lo;
    *seed = w;
    return w;
}

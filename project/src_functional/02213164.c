/* fixed_sqrt16 [0x02213164]: Digit-by-digit integer square root with fixed-point result scaling. */
#include "dh_band21.h"
/* 0x02213164 (sqrt_16_16) - digit-by-digit integer square root, 2 bits per
 * iteration over the 32-bit input (16 iterations). Non-restoring style: at each
 * step the two top bits of x are folded into the running remainder, the root is
 * doubled, and when the trial value fits it is subtracted and the root bumped.
 * Returns (root >> 1) << 8, i.e. the 16.16->fixed sqrt scaled by 256. Leaf.
 * fidelity: HIGH - exact loop (19..3 = 16 iters), rotl pair captures bits 31..30,
 *   cmp/hs branch sense, delay-slot decrement, and final shlr/shll8 all modelled. */
u32 fixed_sqrt16(u32 x)
{
    u32 root = 0;   /* r0 */
    u32 rem  = 0;   /* r3 */
    s32 i    = 19;  /* r1 */

    do {
        u32 top2;
        x = (x << 1) | (x >> 31);   /* rotl */
        x = (x << 1) | (x >> 31);   /* rotl */
        top2 = x & 3;
        rem  = (rem << 2) | top2;
        root <<= 1;
        i--;                        /* delay slot: always executes */
        if (root < rem) {           /* cmp/hs false => r0 < r3 */
            root++;
            rem -= root;
            root++;
        }
    } while (i != 3);

    return (root >> 1) << 8;
}

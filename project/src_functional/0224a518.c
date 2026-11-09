/* shc_double_subtract [0x0224a518]: Soft-double add/sub core with stack-passed register-pair ABI; v1.09 SKIP preserved. */
#include "dh.h"
/* 0x0224a518  dsub - compiler runtime soft-float DOUBLE subtract: *result = a - b.
 * (Earlier notes guessed __divdf3; the body is the add/sub core, not a divide.)
 *
 * ABI (NON-STANDARD, stack-passed, callee-pops, all registers preserved):
 *   @(0,sp)  = u32 *result     (8-byte slot, written hi word then lo word)
 *   @(4,sp)  = a.hi  @(8,sp)  = a.lo
 *   @(12,sp) = b.hi  @(16,sp) = b.lo
 *   returns with sp += 20.  Doubles are IEEE-754 big-endian word pairs.
 * The only caller (0x0223d768, effect_bounce_task) pushes 1296.0
 * (0x40944000:00000000) as b and itod(entity->unk70) as a, then dtoi()s the
 * result, i.e. unk70 = (s32)((f64)unk70 - 1296.0).
 *
 * The same body has two more entry points that nothing in the ROM calls:
 *   0x0224a530  *result = b - a      0x0224a5e0  *result = a + b
 *
 * ALGORITHM (one shared add core at 0x24a5f2; this entry flips b's sign):
 *   - order the operands by the hi word only (rotl-compare: magnitude, then
 *     sign as the tie-break), so A is not smaller than B in its hi word;
 *   - unpack 11-bit exponents and 52-bit mantissas; specials:
 *       expA == 0x7FF: A NaN -> 0x7FF00000:00000001; A inf: B inf (lo==0)
 *         with opposite sign -> same NaN, else +-inf with A's sign;
 *       expB == 0: B zero -> A unchanged; A and B both zero -> +-0 with
 *         sign = signA & signB; A zero -> B; denormals normalised first;
 *   - mantissas get the implicit bit and 3 guard bits (<<3, bit 55);
 *   - B is aligned right by expA-expB with a sticky bit (>54 -> sticky only;
 *     a non-positive difference, possible only with two denormals, does no
 *     shift at all);
 *   - same signs: add, one sticky right shift on carry-out, overflow -> inf;
 *     different signs: exact cancellation -> +0, else subtract (negate and take
 *     B's sign on borrow) and normalise (no sticky needed);
 *   - exponent <= 0: denormalise by 1-exp plain (non-sticky) right shifts;
 *   - round to nearest even on the 3 guard bits, renormalise on carry;
 *   - pack: drop guard bits, mask the 20 hi mantissa bits (a denormal that
 *     rounds up into bit 20 loses it - original quirk, kept), insert exponent
 *     and sign.
 * For normal operands and results this is IEEE round-to-nearest-even, so it
 * equals host double subtraction; NaN encoding and denormal-result rounding
 * differ, hence the bit-level model below instead of `a - b`.
 *
 * fidelity: HIGH - step-for-step integer model of the ROM routine, checked
 * bit-exact against the original in the SH-2 emulator. */
void shc_double_subtract(u32 *result, u32 a_hi, u32 a_lo, u32 b_hi, u32 b_lo)
{
    u32 ah = a_hi, al = a_lo;
    u32 bh = b_hi ^ 0x80000000u, bl = b_lo;
    u32 sa, sb, hi;
    s32 ea, eb, d;
    u64 ma, mb, m;

    if (((ah << 1) | (ah >> 31)) < ((bh << 1) | (bh >> 31))) {
        u32 t;
        t = ah; ah = bh; bh = t;
        t = al; al = bl; bl = t;
    }
    sa = ah;
    sb = bh;
    ea = (s32)((ah >> 20) & 0x7FF);
    eb = (s32)((bh >> 20) & 0x7FF);
    ma = ((u64)(ah & 0xFFFFF) << 32) | al;
    mb = ((u64)(bh & 0xFFFFF) << 32) | bl;

    if (ea == 0x7FF) {
        if (ma != 0 ||
            (eb == 0x7FF && (bl != 0 || ((sa ^ sb) & 0x80000000u)))) {
            sa = 0;                     /* NaN: 0x7FF00000:00000001 */
            m = 8;
        } else {
            m = ma;                     /* +-inf, A's sign (not pre-shifted) */
        }
        goto pack;
    }
    if (eb == 0) {
        if (mb == 0) {
            if (ea == 0 && ma == 0) {   /* +-0 +- 0 */
                sa &= sb;
                m = 0;
                goto pack;
            }
            m = ma << 3;                /* x - 0 = x */
            goto pack;
        }
        if (ea == 0) {
            if (ma == 0) {              /* 0 - y = -y (B carries the flip) */
                m = mb << 3;
                ea = eb;
                sa = sb;
                goto pack;
            }
            ma <<= 1;                   /* normalise denormal A */
            while (ma < (1ULL << 52)) {
                ea--;
                ma <<= 1;
            }
        }
        mb <<= 1;                       /* normalise denormal B */
        while (mb < (1ULL << 52)) {
            eb--;
            mb <<= 1;
        }
    }

    ma = (ma << 3) | (1ULL << 55);
    mb = (mb << 3) | (1ULL << 55);
    d = ea - eb;
    if (d != 0) {
        if (d > 54) {
            mb = 1;
        } else {
            if (d >= 32) {
                d -= 32;
                mb = (mb >> 32) | ((u32)mb != 0);
            }
            if (d >= 16) {
                d -= 16;
                mb = (mb >> 16) | ((mb & 0xFFFF) != 0);
            }
            while (d > 0) {
                mb = (mb >> 1) | (mb & 1);
                d--;
            }
        }
    }

    if (((sa ^ sb) & 0x80000000u) == 0) {
        m = ma + mb;
        if ((m >> 32) >= 0x1000000) {
            m = (m >> 1) | (m & 1);
            ea++;
            if (ea == 0x7FF) {          /* overflow -> inf */
                m = 0;
                goto pack;
            }
        }
    } else {
        if (ma == mb) {                 /* exact cancellation -> +0 */
            sa = 0;
            ea = 0;
            m = 0;
            goto pack;
        }
        m = ma - mb;
        if (ma < mb) {
            m = 0 - m;
            sa = sb;
        }
        if ((m >> 32) == 0) {
            m <<= 32;
            ea -= 32;
        }
        if (((m >> 32) & 0xFFFF0000u) == 0) {
            m <<= 16;
            ea -= 16;
        }
        while ((m >> 32) >= 0x1000000) {
            m >>= 1;
            ea++;
        }
        while ((m >> 32) < 0x800000) {
            m <<= 1;
            ea--;
        }
    }

    if (ea <= 0) {                      /* denormal result */
        m >>= 1;
        while (ea < 0) {
            m >>= 1;
            ea++;
        }
    }
    if ((m & 4) && (m & 0xB)) {         /* round to nearest even */
        m += 8;
        if ((m >> 32) >= 0x1000000) {
            m >>= 1;
            ea++;
        }
    }

pack:
    m >>= 3;
    hi = ((u32)(m >> 32) & 0xFFFFF) | ((u32)ea << 20);
    result[0] = (hi & 0x7FFFFFFFu) | (sa & 0x80000000u);
    result[1] = (u32)m;
}

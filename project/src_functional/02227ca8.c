/* actor_build_proximity_tables [0x02227ca8]: Rebuilds actor/actor and actor/effect squared-distance tables and nearest-slot indices. */
#include "dh_band21.h"
/* 0x02227ca8 - per-frame player/target proximity tables (void; incoming r5 is a purge length).
 * Only caller: bsrf at 0x0220135e (step 9 of the frame pipeline at 0x0220134c,
 * between displaylist_radix_sort 0x02214e2c and the 0x022013c4 step). That
 * caller has no C yet, so no C file references this function.
 *
 * Two tables, both slot-indexed 1..7 (slot 0 unused):
 *   A = 0x06099584: +0x0C+s*4 slot-active word, +0x2C+s*4 entity ptr,
 *       +0x4C [8][8] u32 squared-distance matrix (row stride 32),
 *       +s (byte) nearest-slot index for row s.
 *   B = 0x060E0E88: +0xA4+m*4 target-active word, +0xC4+m*4 entity ptr,
 *       +0xE4 [8][8] u32 slot->target squared-distance matrix (row = slot).
 * A slot's position is the fixed vec 0x06099764 + (s8)((s-1)*12) when its bit
 * g_0604ebe4[s-1] is set in the mask *0x0609977C, else entity->+8 (ptr) + 8.
 * For each slot i: row/column i of A's matrix vs slots j>i are filled with
 * dist2_xyz (0x022131fc) or -1 when j is absent; then the nearest slot over
 * k=1..6 (unsigned <, k=7 never considered, dist[i][i] included as-is) is
 * stored in A[i]; then row i of B's matrix gets dist2_xz (0x022131d4) to every
 * active target m=0..7, else -1. An absent slot i gets -1 in its A row/col
 * (j>i) and in all of its B row, and A[i] is left untouched.
 *
 * REAL REGISTER ABI (why this was floored as "nonstd-abi"): the function itself
 * is a normal SHC void(void) with r8-r14/pr saved. The oddity is its prologue,
 * which passes two structs BY VALUE to cache-purge 0x022148ac:
 *   - 0x0224a280 is SHC's struct-copy helper with a register ABI of its own:
 *     r0 = byte count, r1 = dst, r2 = src (here dst = fresh stack area,
 *     src = A with count 0x14C, then src = B with count 0x1E4);
 *   - then 0x022148ac(r4 = 0x14C / 0x1E4 = the size, r5 = never set here).
 *     The source evidently wrote purge(*A, sizeof *A): the struct went on the
 *     stack (popped straight after the call, never read) and the size landed in
 *     r4, where 0x022148ac expects the BASE address. r5 (its length) is whatever
 *     the caller left: for the first call that is r5 from 0x02214e2c, which ends
 *     with r5 = 0 whenever any radix bucket was non-empty; for the second call
 *     it is 0, because 0x022148ac clears r5 in its own exit path. With len 0 the
 *     purge loop does nothing, so both calls are no-ops on hardware.
 *   This C exposes incoming r5 as its second word argument and keeps both copies.
 *   The first purge receives that incoming length; the second receives zero,
 *   matching the purge leaf's r5=0 branch-delay assignment. work/decomp5/proximity_abi.asm.
 *   CORNER: if the caller ever arrived with r5 != 0, the first call would zero
 *   one word per 16 bytes over [0x40000140, 0x40000140+r5) - SH-2 cache
 *   address-array purge space, not RAM. Not modelled.
 * fidelity: HIGH for the table logic (store order dist[j][i] before dist[i][j],
 *   unsigned min, k range 1..6 and the (s8) offset cast all as in the asm). */
extern void shc_copy_word_block(s32 count, void *dst, void *src); /* struct copy (r0/r1/r2) */
extern void cache_clear_line_heads(u32 base, u32 len);              /* cache purge, see above */
extern s32  distance_squared_xyz(void *a, void *b);                /* dist2_xyz_fixed */
extern s32  distance_squared_xz(void *a, void *b);                /* dist2_xz_fixed */

#define TBL_A      DH_ACTOR_SLOT_REGISTRY_ADDR_U
#define TBL_B      0x060E0E88u
#define SLOT_MASKS DH_ACTOR_SLOT_MASKS_ADDR_U
#define SLOT_MASK  0x0609977Cu
#define SLOT_VECS  0x06099764u

static u32 slot_pos(s32 s)
{
    if (MEM32(SLOT_MASKS + (s - 1) * 4) & MEM32(SLOT_MASK))
        return SLOT_VECS + (s8)((s - 1) * 12);
    return MEMU32(MEMU32(TBL_A + 0x2C + s * 4) + 8) + 8;
}

static s32 slot_present(s32 s)
{
    return MEM32(TBL_A + 0x0C + s * 4) != 0
        || (MEM32(SLOT_MASKS + (s - 1) * 4) & MEM32(SLOT_MASK)) != 0;
}

void actor_build_proximity_tables(u32 unused_r4, u32 incoming_purge_length)
{
    u32 copy_a[0x14C / 4], copy_b[0x1E4 / 4];
    s32 i, j, k, m;

    (void)unused_r4; /* ROM preserves the incoming r5 for its first purge call. */
    shc_copy_word_block(0x14C, copy_a, (void *)(uintptr_t)TBL_A);
    cache_clear_line_heads(0x14C, incoming_purge_length);
    shc_copy_word_block(0x1E4, copy_b, (void *)(uintptr_t)TBL_B);
    cache_clear_line_heads(0x1E4, 0);

    for (i = 1; i < 8; i++) {
        u32 row_a = TBL_A + 0x4C + i * 32;
        u32 row_b = TBL_B + 0xE4 + i * 32;

        if (!slot_present(i)) {
            for (j = i + 1; j < 8; j++) {
                MEM32(TBL_A + 0x4C + j * 32 + i * 4) = -1;
                MEM32(row_a + j * 4) = -1;
            }
            for (m = 0; m < 8; m++)
                MEM32(row_b + m * 4) = -1;
            continue;
        }

        {
            u32 pos_i = slot_pos(i);
            u32 best;

            for (j = i + 1; j < 8; j++) {
                s32 d = -1;
                if (slot_present(j))
                    d = distance_squared_xyz((void *)(uintptr_t)pos_i,
                                      (void *)(uintptr_t)slot_pos(j));
                MEM32(TBL_A + 0x4C + j * 32 + i * 4) = d;
                MEM32(row_a + j * 4) = d;
            }

            MEM8(TBL_A + i) = 0;
            best = 0xFFFFFFFFu;
            for (k = 1; k < 7; k++) {
                if (MEMU32(row_a + k * 4) < best) {
                    MEM8(TBL_A + i) = (u8)k;
                    best = MEMU32(row_a + k * 4);
                }
            }

            for (m = 0; m < 8; m++) {
                s32 d = -1;
                if (MEM32(TBL_B + 0xA4 + m * 4) != 0)
                    d = distance_squared_xz((void *)(uintptr_t)pos_i,
                                      (void *)(uintptr_t)(MEMU32(MEMU32(TBL_B + 0xC4 + m * 4) + 8) + 8));
                MEM32(row_b + m * 4) = d;
            }
        }
    }
}

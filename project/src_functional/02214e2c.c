/* display_radix_sort_records [0x02214e2c]: Stably sorts deferred display records by their twenty-four-bit depth key. */
/* 0x02214e2c (displaylist_radix_sort) - LSD radix sort of the deferred display
 * records by the 24-bit sort key held in rec[0x20], so they are later walked in
 * priority order. Three stable passes over bucket-head tables (256 buckets each,
 * 12-byte stride) using a bump allocator of 12-byte link nodes {next, rec, key}:
 *   pass 1 : bucket records in [0x060c3cc4 .. *0x060cf0c4) by key byte 0 (& 0xff),
 *            walked high->low (stride 0x24) into table @0x060bacc0 (nodes @0x060bb8c0);
 *            then the second, uncached record list (*0x260cf0c8 + 0x20000024 ..
 *            0x260cf0c4) is bucketed into the same table.
 *   pass 2 : drain table @0x060bacc0 into table @0x060bf4c0 by key byte 1 ((&0xff00)>>8),
 *            nodes @0x060c00c0; clears each source bucket.
 *   pass 3 : reset node allocator to 0x060bb8c0 and drain table @0x060bf4c0 back into
 *            table @0x060bacc0 by key byte 2 ((&0x3f0000)>>16); clears each source bucket.
 * Final sorted singly-linked lists live in table @0x060bacc0 (node = {next, rec}).
 * fidelity: faithful - 12-byte node stride, bump allocators @0x060bacb8/0x060bacbc,
 *   key masks 0xff / 0xff00 / 0x3f0000, bucket stride 12, no external calls. */
#include "dh_band21.h"

void display_radix_sort_records(void) {
    u32 rec, cur, node, free, bp, b;
    s32 key, i;

    MEM32(0x060bacb8) = 0x060bb8c0;   /* node allocator for table @0x060bacc0 */
    MEM32(0x060bacbc) = 0x060c00c0;   /* node allocator for table @0x060bf4c0 */

    /* pass 1: bucket the primary record list by key byte 0 */
    rec = MEM32(0x060cf0c4);
    for (;;) {
        rec -= 0x24;
        if (rec < 0x060c3cc4U) break;
        key  = MEM32(rec + 0x20) & 0xff;
        free = MEM32(0x060bacb8);
        MEM32(0x060bacb8) = free + 0xc;
        node = free;
        b = 0x060bacc0 + (u32)(key * 0xc);
        MEM32(node + 0) = MEM32(b);
        MEM32(node + 4) = rec;
        MEM32(node + 8) = MEM32(rec + 0x20);
        MEM32(b) = node;
    }

    /* pass 1b: bucket the secondary (uncached) record list by key byte 0 */
    cur = MEM32(0x260cf0c8) + 0x20000024;
    while (cur < 0x260cf0c4U) {
        key  = MEM32(cur + 0x20) & 0xff;
        free = MEM32(0x060bacb8);
        MEM32(0x060bacb8) = free + 0xc;
        node = free;
        b = 0x060bacc0 + (u32)(key * 0xc);
        MEM32(node + 0) = MEM32(b);
        MEM32(node + 4) = cur;
        MEM32(node + 8) = MEM32(cur + 0x20);
        MEM32(b) = node;
        cur += 0x24;
    }

    /* pass 2: drain table @0x060bacc0 into table @0x060bf4c0 by key byte 1 */
    bp = 0x060bacc0;
    for (i = 0x100; i != 0; i--) {
        u32 n = MEM32(bp);
        if (n != 0) {
            do {
                key  = (MEM32(n + 8) & 0x0000ff00) >> 8;
                free = MEM32(0x060bacbc);
                MEM32(0x060bacbc) = free + 0xc;
                node = free;
                b = 0x060bf4c0 + (u32)(key * 0xc);
                MEM32(node + 0) = MEM32(b);
                MEM32(node + 4) = MEM32(n + 4);
                MEM32(node + 8) = MEM32(n + 8);
                MEM32(b) = node;
                n = MEM32(n + 0);
            } while (n != 0);
            MEM32(bp) = 0;
        }
        bp += 0xc;
    }

    /* pass 3: drain table @0x060bf4c0 back into table @0x060bacc0 by key byte 2 */
    MEM32(0x060bacb8) = 0x060bb8c0;
    bp = 0x060bf4c0;
    for (i = 0x100; i != 0; i--) {
        u32 n = MEM32(bp);
        if (n != 0) {
            do {
                key  = (MEM32(n + 8) & 0x003f0000) >> 16;
                free = MEM32(0x060bacb8);
                MEM32(0x060bacb8) = free + 0xc;
                node = free;
                b = 0x060bacc0 + (u32)(key * 0xc);
                MEM32(node + 0) = MEM32(b);
                MEM32(node + 4) = MEM32(n + 4);
                MEM32(b) = node;
                n = MEM32(n + 0);
            } while (n != 0);
            MEM32(bp) = 0;
        }
        bp += 0xc;
    }
}

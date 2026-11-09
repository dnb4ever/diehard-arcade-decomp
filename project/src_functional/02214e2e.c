/* display_radix_sort_entities [0x02214e2e]: Builds three stable byte-wise sort passes through the two entity bucket arenas. */
#include "dh.h"
/* 0x02214e2e - 3-pass LSD radix sort of the scene's entity records into draw-order buckets.
 * Entity records are 0x24 bytes with a 32-bit sort key at +0x20. Two bucket tables of 0x100
 * heads x 0xC bytes are used (b1 @0x060BACC0 with node arena @0x060BB8C0, b2 @0x060BF4C0 with
 * node arena @0x060C00C0); each list node is 0xC bytes {next@0, entity@4, key@8}. Pass 1/2 bucket
 * both entity arrays (the primary list @[0x060C3CC4 .. *0x060CF0C4) walked backward, and the
 * uncached mirror list below *0x260CF0C8 walked forward) by key bits [7:0] into b1. Pass 3 moves
 * b1 -> b2 by key bits [15:8], then b2 -> b1 by key bits [21:16], clearing each source bucket.
 * The final sorted order is the b1 linked lists.
 * fidelity: HIGH - byte strides (node 0xC, entity 0x24, bucket 0xC) and widths taken from the
 * loads; key read 32-bit, digit extracts logical-shifted. The array-2 walk keeps the ROM's
 * literal uncached-mirror addressing (0x26xxxxxx = 0x06xxxxxx | 0x20000000). No braf/bsrf. */
extern s8  g_060BACC0[];   /* bucket table b1 (0x100 x 0xC) */
extern s8  g_060BF4C0[];   /* bucket table b2 (0x100 x 0xC) */
extern s8 *g_060BACB8;     /* b1 node-arena bump pointer */
extern s8 *g_060BACBC;     /* b2 node-arena bump pointer */
extern s8 *g_060CF0C4;     /* end pointer of entity array 1 */

/* Allocate a fresh 0xC-byte node to bucket[digit] of `table`, bumping `*arena`.
 * The caller publishes the bucket head after filling the node (06018e8e..e96).
 * Returns the node; caller fills +0x4 (entity) and, except in the last pass, +0x8 (key). */
static s8 *allocate_bucket_node(s8 *table, s32 digit, s8 **arena) {
    s8 *slot = &table[digit * 0xC];
    s8 *node = *arena;
    *arena = node + 0xC;
    MP(node, 0x0) = MP(slot, 0x0);
    return node;
}

void display_radix_sort_entities(void) {
    s8 *e;
    s8 *b;
    s8 *node;
    s32 i;

    g_060BACB8 = (s8 *)0x060BB8C0;
    g_060BACBC = (s8 *)0x060C00C0;

    /* pass 1: entity array 1, walked from the end downward, keyed by bits [7:0] */
    e = g_060CF0C4;
    for (;;) {
        e -= 0x24;
        if (e < (s8 *)0x060C3CC4)
            break;
        s32 digit = M32(e, 0x20) & 0xFF;
        node = allocate_bucket_node(g_060BACC0, digit, &g_060BACB8);
        MP(node, 0x4) = e;
        M32(node, 0x8) = M32(e, 0x20);
        MP(g_060BACC0 + digit * 0xC, 0) = node;
    }

    /* pass 2: entity array 2 (uncached mirror), walked upward, keyed by bits [7:0] */
    e = *(s8 **)0x260CF0C8 + 0x20000000 + 0x24;
    while (e < (s8 *)0x260CF0C4) {
        s32 digit = M32(e, 0x20) & 0xFF;
        node = allocate_bucket_node(g_060BACC0, digit, &g_060BACB8);
        MP(node, 0x4) = e;
        M32(node, 0x8) = M32(e, 0x20);
        MP(g_060BACC0 + digit * 0xC, 0) = node;
        e += 0x24;
    }

    /* pass 3a: b1 -> b2, keyed by bits [15:8] */
    b = g_060BACC0;
    for (i = 0x100; i != 0; i--) {
        node = MP(b, 0x0);
        if (node != 0) {
            do {
                s32 digit = (MU32(node, 0x8) & 0xFF00) >> 8;
                s8 *dst = allocate_bucket_node(g_060BF4C0, digit, &g_060BACBC);
                MP(dst, 0x4) = MP(node, 0x4);
                M32(dst, 0x8) = M32(node, 0x8);
                MP(g_060BF4C0 + digit * 0xC, 0) = dst;
                node = MP(node, 0x0);
            } while (node != 0);
            MP(b, 0x0) = 0;
        }
        b += 0xC;
    }

    /* pass 3b: b2 -> b1, keyed by bits [21:16]; node key (+0x8) is not re-stored here */
    b = g_060BF4C0;
    g_060BACB8 = (s8 *)0x060BB8C0;
    for (i = 0x100; i != 0; i--) {
        node = MP(b, 0x0);
        if (node != 0) {
            do {
                s32 digit = (MU32(node, 0x8) & 0x3F0000) >> 16;
                s8 *dst = allocate_bucket_node(g_060BACC0, digit, &g_060BACB8);
                MP(dst, 0x4) = MP(node, 0x4);
                MP(g_060BACC0 + digit * 0xC, 0) = dst;
                node = MP(node, 0x0);
            } while (node != 0);
            MP(b, 0x0) = 0;
        }
        b += 0xC;
    }
}

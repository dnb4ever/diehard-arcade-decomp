/* 0x022157ba  build_displist_a: walk the source table @0x060dc648 (48-byte stride) until an
 * entry's s32[+32] == -1, emitting a 36-byte command per entry into *(void**)0x060cf0c4:
 *   dst[+32] = src[+32] + 0x200000
 *   dst.s16[+0] = 5;  dst.s16[+4] = 0x4c0
 *   p = (void*)src[+40];  v = *(s32*)(p + 8);  dst.s16[+6] = (v == 0) ? 1 : v
 *   dst.s32[+12..+24] = src.s32[+0..+12]
 *   (*0x0609881c)++   [asm reads the 0x2609881c mirror]; slot += 36
 * fidelity: faithful - bra-to-condition loop, cmp/eq #-1 sentinel, field copies, counter++. Leaf. */
#include "dh_band21.h"

void build_displist_a(void) {
    s8 *src = PTR(s8, 0x060dc648);
    s8 **slot = PTR(s8 *, 0x060cf0c4);

    while (*(s32 *)(src + 32) != -1) {
        s8 *dst = *slot;
        s8 *p;
        s32 v;

        *(s32 *)(dst + 32) = *(s32 *)(src + 32) + 0x200000;
        *(s16 *)(dst + 0) = 5;
        *(s16 *)(dst + 4) = 0x4c0;

        p = *(s8 **)(src + 40);
        v = *(s32 *)(p + 8);
        *(s16 *)(dst + 6) = (s16)((v == 0) ? 1 : v);

        *(s32 *)(dst + 12) = *(s32 *)(src + 0);
        *(s32 *)(dst + 16) = *(s32 *)(src + 4);
        *(s32 *)(dst + 20) = *(s32 *)(src + 8);
        *(s32 *)(dst + 24) = *(s32 *)(src + 12);

        MEM32(0x0609881c) = MEM32(0x0609881c) + 1;
        *slot = dst + 36;
        src += 48;
    }
}

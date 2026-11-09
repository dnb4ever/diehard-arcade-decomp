/* actor_decode_enemy_direction [0x02245e58]: Derives the two-bit direction code from animation events and facing yaw. */
#include "dh.h"
/* 0x02245e58  derive direction code (field[0x4d]) from field[0x54] bits 4-5.
 * v = (field[0x54] & 0x30) >> 4; if (field[0x26] == 0x4000) v ^= 2;
 * field[0x4d] = v.
 * fidelity: HIGH - exact (extu.w zero-extend, shar x4 = >>4, compare is 16-bit). */
void actor_decode_enemy_direction(void *p) {
    s32 v = (MU16(p, 0x54) & 0x30) >> 4;
    if ((u16)M16(p, 0x26) == 0x4000)
        v ^= 2;
    M8(p, 0x4D) = (s8)v;
}

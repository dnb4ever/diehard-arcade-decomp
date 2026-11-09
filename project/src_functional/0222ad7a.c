/* actor_decode_pose_direction [0x0222ad7a]: Derives a two-bit direction code from animation events and facing yaw. */
#include "dh.h"
/* 0x0222ad7a  derive a 2-bit facing value from flags@0x54, invert bit1 when angle@0x26 == 0x4000;
 * store to byte 0x4D.
 * fidelity: HIGH - exact (v = (field0x54 & 0x30) >> 4; conditional bit1 flip; -3 == ~2). */
void actor_decode_pose_direction(void *arg0) {
    s32 v = (s32)(0x30 & MU16(arg0, 0x54)) >> 4;
    if (MU16(arg0, 0x26) == 0x4000) {
        v = (~v & 2) | (v & ~2);
    }
    M8(arg0, 0x4D) = (s8)v;
}

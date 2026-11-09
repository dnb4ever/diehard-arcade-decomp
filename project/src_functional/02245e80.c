/* actor_face_from_yaw_sign [0x02245e80]: Sets facing and cardinal yaw from the current signed yaw. */
#include "dh.h"
/* 0x02245e80  set facing from sign of velocity field[0x26].
 * if field[0x26] >= 0: field[0x44]=1, field[0x26]=0x4000;
 * else:                field[0x44]=0, field[0x26]=0xC000.
 * fidelity: HIGH - exact (cmp/pz = signed >= 0; 0x26 is 16-bit). */
void actor_face_from_yaw_sign(void *p) {
    if (M16(p, 0x26) >= 0) {
        M8(p, 0x44) = 1;
        M16(p, 0x26) = 0x4000;
    } else {
        M8(p, 0x44) = 0;
        M16(p, 0x26) = (s16)0xC000;
    }
}

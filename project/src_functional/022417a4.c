/* actor_apply_direction_code [0x022417a4]: Converts the two-bit direction code into facing and yaw. */
#include "dh.h"
/* 0x022417a4  set facing fields from direction code in byte field[0x4d] (4 cases).
 *   0 -> field[0x44]=0, field[0x26]=0xC000
 *   1 -> field[0x44]=0, field[0x26]=0x4000
 *   2 -> field[0x44]=1, field[0x26]=0x4000
 *   3 -> field[0x44]=1, field[0x26]=0xC000
 * other codes leave fields unchanged.
 * fidelity: HIGH - exact (mov.b = signed byte load; 0x26 stored 16-bit). */
void actor_apply_direction_code(void *p) {
    s8 dir = M8(p, 0x4D);
    switch (dir) {
    case 0: M8(p, 0x44) = 0; M16(p, 0x26) = (s16)0xC000; break;
    case 1: M8(p, 0x44) = 0; M16(p, 0x26) = 0x4000;      break;
    case 2: M8(p, 0x44) = 1; M16(p, 0x26) = 0x4000;      break;
    case 3: M8(p, 0x44) = 1; M16(p, 0x26) = (s16)0xC000; break;
    default: break;
    }
}

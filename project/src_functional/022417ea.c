/* actor_face_direction_code [0x022417ea]: Direction codes 0/1 select heading c000 and facing zero; 2/3 select 4000 and one. */
#include "dh.h"
/* 0x022417ea  set facing fields from direction code in byte field[0x4d].
 * codes 0/1 -> field[0x44]=0, field[0x26]=0xC000; codes 2/3 -> field[0x44]=1,
 * field[0x26]=0x4000; other codes leave fields unchanged.
 * fidelity: HIGH - exact (mov.b = signed byte load; 0x26 stored 16-bit). */
void actor_face_direction_code(void *p) {
    s8 dir = M8(p, 0x4D);
    if (dir == 0 || dir == 1) {
        M8(p, 0x44) = 0;
        M16(p, 0x26) = (s16)0xC000;
    } else if (dir == 2 || dir == 3) {
        M8(p, 0x44) = 1;
        M16(p, 0x26) = 0x4000;
    }
}

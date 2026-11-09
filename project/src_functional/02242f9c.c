/* actor_request_animation47e [0x02242f9c]: Sets animation_control=1, requested animation=0x47e and word+bc=-1. */
#include "dh.h"
/* 0x02242f9c  small init: field[0x41]=1; field[0x5c]=0x47E; field[0xbc]=-1.
 * fidelity: HIGH - exact (0x5c stored 16-bit, 0xbc stored 32-bit). */
void actor_request_animation47e(void *p) {
    M8(p, 0x41) = 1;
    M16(p, 0x5C) = 0x47E;
    M32(p, 0xBC) = -1;
}

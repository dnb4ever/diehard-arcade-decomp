/* players_publish_health_order [0x0222a0ec]: Writes ordering 2/equal, 1/less, 0/greater to 0609856c. */
#include "dh.h"
/* 0x0222a0ec  compare field 0x56 of two actors reached via 0x060995B0[+4]->+8 and [+8]->+8;
 * store ordering result (2 equal / 1 less / 0 greater) to 0x0609856C.
 * fidelity: HIGH - exact; field 0x56 is 16-bit, signed compare. */
extern u8  g_actor_slot_tasks[];
extern s32 g_0609856C;
void players_publish_health_order(void) {
    void *a  = MP(&g_actor_slot_tasks[0x4], 0);
    void *a2 = MP(a, 0x8);
    void *b  = MP(&g_actor_slot_tasks[0x8], 0);
    void *b2 = MP(b, 0x8);
    s16 va = M16(a2, 0x56);
    s16 vb = M16(b2, 0x56);
    if (va == vb) {
        g_0609856C = 2;
    } else if (va < vb) {
        g_0609856C = 1;
    } else {
        g_0609856C = 0;
    }
}

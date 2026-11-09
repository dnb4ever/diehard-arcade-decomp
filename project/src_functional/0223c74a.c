/* actor_consume_linked_effect_count [0x0223c74a]: Decrements the linked effect's halfword count or returns minus one when unavailable. */
#include "dh.h"
/* 0x0223c74a (callC) - returns s32.
 * If *(s8)(arg0+109) == 0 return -1.
 * entry = *( 0x060E0F4C[ *(s8)(arg0+111) ] + 8 ).
 * If entry.w@86 == 0 return -1. Else decrement entry.w@86; return 0.
 * fidelity: HIGH - field 86 16-bit (mov.w); ptr table stride 4; return in r0. */
extern s32 g_060E0F4C;
s32 actor_consume_linked_effect_count(void *arg0) {
    void *e;
    if (M8(arg0, 109) == 0)
        return -1;
    e = MP(MP(&g_060E0F4C, 4 * M8(arg0, 111)), 8);
    if (M16(e, 86) == 0)
        return -1;
    M16(e, 86) = (s16)(M16(e, 86) - 1);
    return 0;
}

/* player_consume_linked_effect_count [0x0223c7ba]: Tests the linked effect count and decrements it only for player slots. */
#include "dh.h"
/* 0x0223c7ba (leafC) - returns s32.
 * If *(s8)(arg0+109) == 0 return -1.
 * entry = *( *(0x060E0F4C + 4*(s8)(arg0+111)) + 8 ).
 * If entry.w@86 == 0 return -1.
 * Else if *(s8)(arg0+52) < 3, decrement entry.w@86; return 0.
 * fidelity: HIGH - field 86 16-bit (mov.w); ptr table 4-byte; return in r0.
 * bf on tst r1,r1: proceed when byte@109 != 0, return -1 when zero. */
extern s32 g_060E0F4C;
s32 player_consume_linked_effect_count(void *arg0) {
    void *e;
    if (M8(arg0, 109) == 0)
        return -1;
    e = MP(MP(&g_060E0F4C, 4 * M8(arg0, 111)), 8);
    if (M16(e, 86) == 0)
        return -1;
    if (M8(arg0, 52) < 3)
        M16(e, 86) = (s16)(M16(e, 86) - 1);
    return 0;
}

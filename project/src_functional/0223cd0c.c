/* effect_pool_release [0x0223cd0c]: Releases an occupied effect slot and subtracts its type's contribution to the shared count. */
#include "dh.h"
/* 0x0223cd0c (leafC) - release slot idx=*(s8)(arg0+52) in table 0x060E0E88.
 * v = base[0x84 + 4*idx]; if 0, return. Subtract word 0x0608CF18[v] from counter at
 * 0x060E1214; decrement 32-bit *0x060E0E88; clear base+0x84/+0xA4/+0xC4 (+4*idx).
 * fidelity: HIGH - v slots 32-bit (stride 4); word table 0x0608CF18 stride 2; counter 16-bit. */
extern s32 g_060E0E88;
extern s16 g_0608CF18;
extern s16 g_060E1214;
void effect_pool_release(void *arg0) {
    void *base = &g_060E0E88;
    s32 idx = M8(arg0, 52);
    s32 v = M32(base, 0x84 + idx * 4);
    if (v == 0)
        return;
    g_060E1214 = (s16)(g_060E1214 - (&g_0608CF18)[v]);
    M32(base, 0) = M32(base, 0) - 1;
    M32(base, 0x84 + idx * 4) = 0;
    M32(base, 0xA4 + idx * 4) = 0;
    M32(base, 0xC4 + idx * 4) = 0;
}

/* player_reset_join_slot [0x0220581a]: Clears the selected player's attract/join state with the original resident byte-address behavior. */
#include "dh.h"
/* 0x0220581a (player_slot_reset) - clears one player's attract/join state. Called as
 * func_0220581a(0) / func_0220581a(1) by the scene bootstrap func_022058a8. p is the player
 * index (0 or 1); the routine indexes the parallel per-player global arrays by p.
 *
 * Verbatim note: the store at `0x0609868c + p` uses the BYTE VALUE read from g_0609868c as its
 * base (mov.b @r3,r0 ; mov.b r5,@(r0,r4)), i.e. the game writes to address ((u8)g_0609868c + p).
 * Reproduced exactly from asm; this is the original (odd) computation, not a transcription error.
 * fidelity: HIGH, hand-traced vs m2c_fn --asm; pools @0x205880..0x2058a4. */

#define R8A(a)   (*(s8  *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define R16A(a)  (*(s16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))

void player_reset_join_slot(s32 p) {
    R8A(0x06098688 + p) = 0;                       /* start flag[p] */
    R8A(0x0609868a + p) = 0;
    R16A(0x06099790 + p * 2) = 0;
    R8A((u8)R8A(0x0609868c) + p) = 0;              /* verbatim: base is the value of g_0609868c */
    R32A(0x0609975c + p * 4) = 30;                 /* accept timer[p] */
    R16A(0x0609978c + p * 2) = R16A(0x06099796);   /* mirror players/region */
    R16A(0x06099788 + p * 2) = R16A(0x06099794);   /* mirror language */
}

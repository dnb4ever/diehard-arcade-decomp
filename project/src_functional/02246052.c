#include "dh_work.h"
/* combat_reset_stage_bounds [0x02246052]: Loads the stage combat bounds and clears pairing, timestamps and combat control globals. */
#include "dh.h"
/* 0x02246052  (re)initialise a batch of HWRAM scalars from a 44-byte record.
 * idx = (*g_0609866c)->field[32]; rec = (u8*)0x0606080c + idx*44;
 *   g_060972d0 = rec[20] + 0x8000;   g_060972d4 = rec[32] - 0x8000;
 *   g_060972d8 = rec[28];            g_060972dc = rec[40];
 *   g_060972a8 = 0;  g_060973ec = 100;  g_06098664 = 0;
 *   g_06099798 = (s16)0;  g_060973e0 = 0;  g_06097410 = 0;  g_06097418 = 0;
 *   g_06099780[0]=0; g_06099780[1]=0;
 *   g_06097420 halfwords [0]=8, [1]=8;  g_06097414 = 0;
 *   g_060973f8 words [0..3] = 0.
 * fidelity: HIGH - exact (mul.l stride 44; signed/16-bit widths per store). */
extern s32 g_0606080c[];   /* record array, 44-byte stride (cart) */
extern void *g_scene_descriptor_legacy;   /* -> owner object */
extern s32 g_060972d0, g_060972d4, g_060972d8, g_060972dc, g_060972a8;
extern s32 g_060973ec, g_06098664, g_060973e0, g_06097410, g_06097418, g_06097414;
extern s16 g_06099798;
extern s32 g_06099780[];
extern u8  g_06097420[];
extern u8  g_060973f8[];
void combat_reset_stage_bounds(void) {
    void *owner = g_scene_descriptor_legacy;
    s32 idx = SCENE_M32(owner, stage_index);
    u8 *rec = (u8 *)g_0606080c + idx * 44;
    g_060972d0 = M32(rec, 20) + 0x8000;
    g_060972d4 = M32(rec, 32) - 0x8000;
    g_060972d8 = M32(rec, 28);
    g_060972dc = M32(rec, 40);
    g_060972a8 = 0;
    g_060973ec = 100;
    g_06098664 = 0;
    g_06099798 = 0;
    g_060973e0 = 0;
    g_06097410 = 0;
    g_06097418 = 0;
    g_06099780[1] = 0;
    g_06099780[0] = 0;
    M16(g_06097420, 2) = 8;
    M16(g_06097420, 0) = 8;
    g_06097414 = 0;
    M32(g_060973f8, 12) = 0;
    M32(g_060973f8, 8) = 0;
    M32(g_060973f8, 4) = 0;
    M32(g_060973f8, 0) = 0;
}

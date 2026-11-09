/* enemy_elapsed_action_threshold [0x02246026]: Unsigned frame-minus-stamp+114 comparison to type/state duration table. */
#include "dh.h"
/* 0x02246026  timer-vs-threshold: has enough time elapsed since stamp field[0x114]?
 * limit = g_0608dc04[field[0x6d]*8 + field[0xe8]];
 * elapsed = g_060987bc - field[0x114];
 * return (elapsed >= limit) ? 1 : 0.
 * fidelity: HIGH - exact (cmp/hs = unsigned >=). */
extern u32 g_frame_tick_legacy;      /* global frame counter */
extern u32 g_0608dc04[];    /* duration/threshold table (cart rodata) */
s32 enemy_elapsed_action_threshold(void *p) {
    u32 limit = g_0608dc04[M8(p, 0x6D) * 8 + M32(p, 0xE8)];
    u32 elapsed = g_frame_tick_legacy - MU32(p, 0x114);
    return (elapsed >= limit) ? 1 : 0;
}

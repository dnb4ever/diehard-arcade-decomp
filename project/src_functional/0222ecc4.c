/* collision_reset_frame [0x0222ecc4]: Clears shared node count, active head and all 512 spatial-grid bucket heads. */
#include "dh.h"
/* 0x0222ecc4  reset the 8-byte-entry stack: clear count/head, zero 0x200 words @0x06097558.
 * fidelity: HIGH - exact (two 32-bit zero stores per loop, 0x100 iterations). */
extern s32   g_collision_node_count;
extern void *g_collision_active_head;
extern s32   g_collision_grid_heads[];
void collision_reset_frame(void) {
    s32 *p = g_collision_grid_heads;
    s32 n = 0x200;
    g_collision_node_count = 0;
    g_collision_active_head = 0;
    do {
        n -= 2;
        p[0] = 0;
        p[1] = 0;
        p += 2;
    } while (n != 0);
}

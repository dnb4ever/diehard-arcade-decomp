/* collision_insert_grid [0x0222eee4]: Hashes actor X/Z into 32x16 buckets and prepends a node from the shared pool. */
#include "dh.h"
/* 0x0222eee4 (spatial_grid_insert) - shared tail dispatcher that the entity-
 * behaviour state machines braf into. Registers the entity into a 32x16 spatial-
 * hash grid for the current frame so later passes can query cell occupancy.
 *
 * Cell is derived from the entity's 16.16 fixed-point world position relative to
 * the grid origin struct at 0x0609742c (+0 = origin X, +8 = origin Y):
 *   col = ((entity->unk08 - originX) >> 16) & 0x1F   (0..31)
 *   row = ((entity->unk10 - originY) >> 16) & 0x0F   (0..15)
 * A node is taken from a 32-entry pool (index = current global count) and pushed
 * to the front of that cell's singly-linked bucket list. Each node is 8 bytes:
 * { next, entity }. When the pool is exhausted (count >= 32) the entity is
 * silently dropped.
 *
 * ABI: r4 = entity pointer. Reads entity->unk08 (world X 16.16) and entity->unk10
 * (world Y 16.16). No return value; side effects only: g_grid_count incremented,
 * one pool node consumed, one bucket head relinked.
 * fidelity: HIGH - verified vs braf_rewrite.py --asm; masks 0x1F/0x0F, row stride
 * 0x80 (0x20 cols * 4), node stride 8, count cap 0x20 all exact. */

struct grid_node { struct grid_node *next; void *entity; };

extern s32  g_collision_node_count;          /* grid node-allocation count (0..32) */
extern u8   g_0609742c[];        /* grid origin: +0 X (16.16), +8 Y (16.16) */
extern struct grid_node *g_collision_grid_heads[]; /* [16][32] bucket head array */
extern struct grid_node  g_collision_node_pool[]; /* 32-entry node pool */

void collision_insert_grid(void *entity)
{
    s32 count = g_collision_node_count;
    s32 col, row;
    struct grid_node **head;
    struct grid_node *node;

    if (count >= 0x20) {
        return;
    }
    g_collision_node_count = count + 1;

    col = (s32)(((u32)(M32(entity, 0x08) - M32(g_0609742c, 0x00)) >> 16) & 0x1F);
    row = (s32)(((u32)(M32(entity, 0x10) - M32(g_0609742c, 0x08)) >> 16) & 0x0F);

    head = &g_collision_grid_heads[row * 0x20 + col];
    node = &g_collision_node_pool[count];
    node->next = *head;
    node->entity = entity;
    *head = node;
}

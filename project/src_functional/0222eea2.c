/* collision_queue_actor [0x0222eea2]: Prepends actor to the active collision list using the shared 32-node pool. */
#include "dh.h"
/* 0x0222eea2 (leafC) - push an entity onto a 32-entry draw/active list. If the count
 * g_06097550 is >= 32, do nothing. Otherwise take node = &g_06097450[count*2] (8-byte nodes),
 * increment the count, link node->next (+0x0) = current head g_06097554, store the entity at
 * node+0x4, and set the head to node.
 * fidelity: HIGH - exact; count/head/links 32-bit, 8-byte nodes (count*8 stride). Leaf. */
extern s32   g_collision_node_count;     /* list element count */
extern s8    g_collision_node_pool[];   /* node pool (8-byte nodes) */
extern void *g_collision_active_head;     /* list head pointer */
void collision_queue_actor(void *entity) {
    s32 count = g_collision_node_count;
    if (count < 32) {
        void *node = &g_collision_node_pool[count * 8];
        g_collision_node_count = count + 1;
        MP(node, 0x0) = g_collision_active_head;
        MP(node, 0x4) = entity;
        g_collision_active_head = node;
    }
}

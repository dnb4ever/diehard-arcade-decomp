/* effect_mark_actor_slot_ready [0x0223a394]: Sets the actor-selected effect slot mode to one. */
#include "dh.h"
/* 0x0223a394 (actor queue-slot mark) - tiny shared helper: marks this actor's command
 * queue slot g_060E0F2C[unk94] = 1 (unk94 is the long slot index).
 * fidelity: HIGH - unk94 long index, store value 1 (long). No branches. */
extern s32 g_060E0F2C[];

void effect_mark_actor_slot_ready(void *entity) {
    g_060E0F2C[M32(entity, 0x94)] = 1;
}

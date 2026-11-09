/* enemy_enter_descriptor_state16 [0x02241e06]: Behavior 16/phase 4, descriptor idle-animation copy and animation 0xe3 request. */
#include "dh_work.h"
#include "dh.h"
/* 0x02241e06  init object and copy an s16 from a two-level pointer table.
 * field[0xd4]=16; field[0xd8]=4. base = g_06091dd4[field[0xe4]] (ptr table);
 * rec = base[field[0x6d]] (ptr); field[0x148] = *(s16*)rec; field[0x41]=1;
 * field[0x5c]=0xE3.
 * fidelity: HIGH - exact; two <<2 index scalings -> word-pointer indexing. */
extern void *g_06091dd4[];  /* table of pointer-tables (cart rodata) */
void enemy_enter_descriptor_state16(void *p) {
    ACTOR_M32(p, behavior_state) = 16;
    ACTOR_M32(p, behavior_phase) = 4;
    void **base = (void **)g_06091dd4[M32(p, 0xE4)];
    void *rec = base[(s8)ACTOR_M8(p, secondary_effect_type)];
    ACTOR_M16(p, idle_animation) = M16(rec, 0);
    M8(p, 0x41) = 1;
    M16(p, 0x5C) = 0xE3;
}

#include "dh_work.h"
/* actor_restore_primary_channel [0x02239f78]: Restores the primary part channel from the saved slot state and clears pending handoff bytes. */
#include "dh.h"
/* 0x02239f78 (leafC) - per-slot reset; idx=*(s8)(arg0+52).
 * (0x06096250 + 32*idx).w = (0x060E11E8 + 4*idx).w; (arg0+0x8C) |= 0x1000000, &= ~0x1000;
 * (arg0+112)=1; (arg0+113)=(arg0+110); (arg0+108)=0; (arg0+110)=0.
 * fidelity: HIGH - slot copy 16-bit (mov.w, strides 32/4); flags 32-bit; 108/110/112/113 8-bit. */
extern s16 g_06096250;
extern s16 g_060E11E8;
void actor_restore_primary_channel(void *arg0) {
    s32 idx = ACTOR_M8(arg0, slot_id);
    M16(&g_06096250, idx * 32) = M16(&g_060E11E8, idx * 4);
    ACTOR_M32(arg0, interaction_flags) |= 0x1000000;
    ACTOR_M32(arg0, interaction_flags) &= ~0x1000;
    ACTOR_M8(arg0, primary_handoff_pending) = 1;
    ACTOR_M8(arg0, primary_handoff_slot) = ACTOR_M8(arg0, primary_effect_slot);
    ACTOR_M8(arg0, primary_effect_type) = 0;
    ACTOR_M8(arg0, primary_effect_slot) = 0;
}

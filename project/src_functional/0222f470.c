/* effect_init_owner_frame_track [0x0222f470]: Copies owner-indexed animation pose and flags; installs tracking continuation. */
#include "dh.h"
/* 0x0222f470  init an entity's animation state from frame record 0x06097D58[src.id<<8 + e.id5F*16]:
 * copy 3 words, id byte, field 0x38, OR in flag table 0x0604EBE4[id], decrement timer 0x58,
 * and install the next state handler (func_0222f4e8) at arg0->unk0.
 * fidelity: HIGH - exact; widths: 0x34=8b, 0x58=16b, rest 32b; 0x5F read unsigned, 0x34 signed. */
extern u8 g_06097D58[]; /* frame/animation record base */
extern u8 g_actor_slot_masks_legacy[]; /* per-id flag table, 4-byte stride */
extern void actor_anim_step_task_source_alias(void *);
void effect_init_owner_frame_track(void *arg0) {
    void *e   = MP(arg0, 0x8);
    void *src = MP(MP(arg0, 0x14), 0x8);
    u8 *rec = &g_06097D58[(M8(src, 0x34) << 8) + MU8(e, 0x5F) * 16];
    M32(e, 0x8)  = M32(rec, 0x0);
    M32(e, 0xC)  = M32(rec, 0x4);
    M32(e, 0x10) = M32(rec, 0x8);
    M8(e, 0x34)  = (s8)M8(src, 0x34);
    M32(e, 0x38) = M32(src, 0x38);
    M32(e, 0x68) = M32(e, 0x68) | M32(&g_actor_slot_masks_legacy[M8(e, 0x34) * 4], 0);
    M16(e, 0x58) = (s16)(M16(e, 0x58) - 1);
    MP(arg0, 0x0) = (void *)actor_anim_step_task_source_alias;
}

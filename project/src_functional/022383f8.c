/* sound_emit_owner_animation_cue [0x022383f8]: Owner animation/model tables select cue byte; gated sound queue request. */
#include "dh.h"
/* 0x022383f8 - sound request dispatch.
 * arg0 = guest address of a record (u32).
 * data = *(u32 *)(*(u32 *)(arg0 + 20) + 8);
 * sp0  = 0x0607D124 + (u16)data[90..91] * 28;
 * sp4  = 0x06054080 + (u8)*(u8 *)(0x060540E8 + (u16)data[94..95]) * 8;
 * if (*(s32 *)0x060E0DDC != 0)
 *     req = ((u16)*(u16 *)(0x06053F04 + (u8)sp4[ (u8)sp0[15] ] * 2) << 8) | 0xA00000FF;
 *     sound_queue_push(req);
 * *(s32 *)arg0 = 0; */
extern volatile s32 g_sound_effects_enabled;
extern void sound_queue_push(s32 req);

void sound_emit_owner_animation_cue(u32 arg0) {
    u32 sp0;
    u32 sp4;
    u32 data;

    data = MU32(MU32(arg0, 20), 8);
    sp0 = 0x0607D124 + MU16(data, 90) * 28;
    sp4 = 0x06054080 + MU8(0x060540E8 + MU16(data, 94), 0) * 8;
    if (g_sound_effects_enabled != 0) {
        sound_queue_push((MU16(0x06053F04 + MU8(sp4, MU8(sp0, 15)) * 2, 0) << 8) | DH_SOUND_CUE_BASE);
    }
    M32(arg0, 0) = 0;
}

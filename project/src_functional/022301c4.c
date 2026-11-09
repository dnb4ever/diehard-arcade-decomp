/* effect_spawn_actor_clone [0x022301c4]: Spawns a list-work clone, copies actor state and position and queues its cue. */
#include "dh.h"
/* 0x022301c4 spawn_clone_from - spawn an entity from template g_06091354[idx] and
 * copy state out of a source entity.
 * args: parent, src, pos, idx.
 *   obj = func_0220ff28(parent, g_06091354[idx])->unk8;
 *   copy src->{unk68,5F,60,62,64,34,38,58,28,14,18,74,22} into obj (same offsets);
 *   obj->unk30 = 0x179;
 *   obj->unk8/C/10 = pos->unk0/4/8;          (world position)
 *   func_02238dde(obj);  obj->unk6C = 8;      (unk6C written in the call's delay slot)
 *   if (g_060E0DDC != 0) sfx_trigger(0xA0130BFF);
 *   g_0609974C[src->unk34] = 0x49;
 * fidelity: HIGH - bsrf resolved: func_0220ff28 (spawn, 0x20ff28), func_02238dde
 * (0x238dde), sfx_trigger (0x215f6c). Field widths per asm (byte 5F/34; word
 * 60/62/58/28/30/22; long 68/64/38/14/18/74/8/C/10/6C). Matches m2c --asm. */
extern void *task_spawn_list_work7c(void *parent, void *template);
extern void  display_mark_channel_bits(void *obj);
extern void  sound_queue_push(u32 id);
extern void *g_06091354[];     /* spawn templates */
extern s32   g_sound_effects_enabled_legacy;
extern s16   g_0609974C[];     /* per-slot status words */

void effect_spawn_actor_clone(void *parent, void *src, void *pos, s32 idx) {
    void *obj = MP(task_spawn_list_work7c(parent, g_06091354[idx]), 0x8);

    M32(obj, 0x68) = M32(src, 0x68);
    M8(obj, 0x5F)  = M8(src, 0x5F);
    M16(obj, 0x60) = M16(src, 0x60);
    M16(obj, 0x62) = M16(src, 0x62);
    M32(obj, 0x64) = M32(src, 0x64);
    M8(obj, 0x34)  = M8(src, 0x34);
    M32(obj, 0x38) = M32(src, 0x38);
    M16(obj, 0x58) = M16(src, 0x58);
    M16(obj, 0x28) = M16(src, 0x28);
    M32(obj, 0x14) = M32(src, 0x14);
    M32(obj, 0x18) = M32(src, 0x18);
    M16(obj, 0x30) = 0x179;
    M32(obj, 0x8)  = M32(pos, 0x0);
    M32(obj, 0xC)  = M32(pos, 0x4);
    M32(obj, 0x10) = M32(pos, 0x8);
    M32(obj, 0x74) = M32(src, 0x74);
    M16(obj, 0x22) = M16(src, 0x22);
    M32(obj, 0x6C) = 8;
    display_mark_channel_bits(obj);
    if (g_sound_effects_enabled_legacy != 0) {
        sound_queue_push(DH_SOUND_CLONE_COMMAND_U);
    }
    g_0609974C[M8(src, 0x34)] = 0x49;
}

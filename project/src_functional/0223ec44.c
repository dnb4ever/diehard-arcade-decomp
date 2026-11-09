/* effect_tick_jittered_emitter [0x0223ec44]: Two-tick cadence spawns jittered 0223ed14 child; every-fourth-frame extra behavior. */
#include "dh.h"
/* func_0223ec44(task, entity): count down entity->unk98; while still positive, do
 * nothing. On expiry reset unk98=2 and spawn a jittered child via
 * func_0220ff28(task, template func_0223ed14). Positions mirror func_0223eb3c but with a
 * fixed high offset (oe->unk8 = (rng & 0x1FFFF) + entity->unk8 - 0x10000) and unk70=4.
 * Every 4th frame (g_060987BC & 3 == 0) with g_060E0DDC set, pushes sound 0xA01406FF.
 * fidelity: MED/HIGH - offsets/shifts verified; func_02210020=rng_next,
 *   func_0220ff28=spawn_obj_0x7c, func_02215f6c=sound_queue_push resolved via braf;
 *   the sound push passes only the explicitly-set id register. */
extern u32   game_random_next(void);
extern void *task_spawn_list_work7c(void *task, void *template);
extern void  sound_queue_push_entry(void *cmd);
extern u8 g_actor_slot_masks_legacy[];
extern u8 g_frame_tick[];
extern u8 g_sound_effects_enabled_legacy[];

void effect_tick_jittered_emitter(void *task, void *ent) {
    s32  old = M32(ent, 0x98);
    u32  r;
    void *obj, *oe;

    M32(ent, 0x98) = old - 1;
    if (old > 0) {
        return;
    }
    M32(ent, 0x98) = 2;
    r   = game_random_next();
    obj = task_spawn_list_work7c(task, (void *)0x06042D14 /* HWRAM handler func_0223ed14 */);
    oe  = MP(obj, 8);
    M32(oe, 8)    = (0x1FFFF & (s32)r) + M32(ent, 8) + (s32)0xFFFF0000;
    M32(oe, 0xC)  = (((s32)r >> 8) & 0x7FFF) + M32(ent, 0xC);
    M32(oe, 0x10) = (s32)(u16)((u32)r >> 16) + M32(ent, 0x10) - 0x8000;
    M32(oe, 0x4C) = M32(oe, 8);
    M32(oe, 0x50) = M32(oe, 0xC);
    M32(oe, 0x54) = M32(oe, 0x10);
    M32(oe, 0x68) = M32(g_actor_slot_masks_legacy, M8(ent, 0x34) * 4);
    M16(oe, 0x60) = 0;
    M8(oe, 0x34)  = M8(ent, 0x34);
    M32(oe, 0x38) = 0x200000;
    M32(oe, 0x64) = 0x4000;
    M32(oe, 0x70) = 4;
    if ((M32(g_frame_tick, 0) & 3) == 0 && M32(g_sound_effects_enabled_legacy, 0) != 0) {
        sound_queue_push_entry((void *)DH_SOUND_EFFECT_VARIANT_COMMAND);
    }
}

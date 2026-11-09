/* effect_spawn_jittered_spark [0x0223eb3c]: RNG-scaled XYZ jitter, copied slot and child handler 0223efec. */
#include "dh.h"
/* func_0223eb3c(task, entity): spawn one jittered child (muzzle/spark). r = rng
 * (func_02210020); obj = func_0220ff28(task, template func_0223efec); oe = obj->unk8.
 * oe position (unk8/C/10) = entity position +- rng jitter scaled by (unkC8<<11), mirrored
 * to oe->unk4C/50/54. Sets oe->unk68 = g_0604EBE4[unk34], unk60=0, unk34=entity->unk34,
 * unk38=0x200000, unk64=0x4000, unk70 = entity->unkC8<<13. Every 8th call
 * (g_060987BC & 7 == 0) with g_060E0DDC set, pushes sound 0xA01402FF.
 * fidelity: MED/HIGH - offsets/shifts verified from asm; func_02210020=rng_next,
 *   func_0220ff28=spawn_obj_0x7c, func_02215f6c=sound_queue_push resolved via braf;
 *   the sound push passes only the explicitly-set id register (r4). */
extern u32   game_random_next(void);                        /* rng_next */
extern void *task_spawn_list_work7c(void *task, void *template);  /* spawn_obj_0x7c */
extern void  sound_queue_push_entry(void *cmd);                   /* sound_queue_push */
extern u8 g_actor_slot_masks_legacy[];
extern u8 g_frame_tick[];
extern u8 g_sound_effects_enabled_legacy[];

void effect_spawn_jittered_spark(void *task, void *ent) {
    u32  r    = game_random_next();
    s32  base = M32(ent, 0xC8) << 11;
    void *obj = task_spawn_list_work7c(task, (void *)0x06042FEC /* HWRAM handler func_0223efec */);
    void *oe  = MP(obj, 8);

    M32(oe, 8)    = ((base - 1) & (s32)r) + M32(ent, 8) - (base >> 1);
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
    M32(oe, 0x70) = M32(ent, 0xC8) << 13;
    if ((M32(g_frame_tick, 0) & 7) == 0 && M32(g_sound_effects_enabled_legacy, 0) != 0) {
        sound_queue_push_entry((void *)DH_SOUND_EFFECT_SPAWN_COMMAND);
    }
}

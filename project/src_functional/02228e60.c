/* player_tick_grab_home_contact [0x02228e60]: Homes paired actor in X/Z; contact spawns linked hit effect and transitions. */
#include "dh_work.h"
#include "dh.h"
/* 0x02228e60 (actor FSM step) - home a paired object (obj = g_060995B0[entity->unk4B]->unk8)
 * toward the actor's target on X(unk8) and Z(unk10). If obj->unk54 bit1 is set (contact):
 * spawn a hit/effect object via func_0220ff72(g_06099580, &D_02226148), link the actor
 * (spawn->unk8->unk10) and the paired object (spawn->unk8->unk14) into it, set state 0x43=0x14
 * and return. Otherwise ease obj->unk8 toward entity->unk8+unkC4 and obj->unk10 toward
 * entity->unk10+unkC8 (halfway each frame), then tail-call animation_update(a0,entity).
 * fidelity: HIGH - all position fields long; obj->unk54 word (unsigned bit test); unk43 byte;
 * shar = signed >>1; spawn type source g_06099580 and the in-ROM descriptor pointer D_02226148
 * taken from the asm (mova + 0xffffd2c4). Computed branches via braf_rewrite.py. */
extern s32   actor_anim_frame_update(void *a0, void *entity);      /* animation_update */
extern void *task_spawn_child_work40(s32 type, void *descriptor);  /* spawn object */
extern s32   g_06099580;
extern void *g_actor_slot_tasks[];
extern u8    D_02226148[];   /* in-ROM spawn descriptor */

void player_tick_grab_home_contact(void *a0, void *entity) {
    void *obj = TASK_PTR(g_actor_slot_tasks[ACTOR_M8(entity, paired_slot)], work);
    if (MU16(obj, 0x54) & 2) {
        void *spawn = task_spawn_child_work40(g_06099580, D_02226148);
        MP(MP(spawn, 0x8), 0x10) = entity;
        MP(MP(spawn, 0x8), 0x14) = obj;
        ACTOR_WORK(entity)->state_handler = 0x14;
        return;
    }
    M32(obj, 0x8)  += ((ACTOR_WORK(entity)->position_x  + M32(entity, 0xC4)) - M32(obj, 0x8))  >> 1;
    M32(obj, 0x10) += ((ACTOR_WORK(entity)->position_z + M32(entity, 0xC8)) - M32(obj, 0x10)) >> 1;
    actor_anim_frame_update(a0, entity);
}

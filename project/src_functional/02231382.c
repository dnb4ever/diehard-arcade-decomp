/* projectile_init_tracking_launch [0x02231382]: Owner animation frame supplies launch position and next tracking handler 0223144a. */
#include "dh_work.h"
#include "dh.h"
/* 0x02231382  projectile_launch_init_task
 *
 * Launch/seed init for a tracked projectile; installs projectile_track_task
 * (0x0223144a) as the next per-frame handler.
 *
 * arg0 = task; ent = task->unk8 (projectile); src = task->unk14->unk8 (owner).
 * Position record rec = &g_06097d58[(src->unk34 << 8) + ((u8)ent->unk5F << 4)].
 *
 * Reserve ammo via func_0223c74a(src); non-zero return kills the task
 * (task->unk0 = 0).  On success: seed ent->unk8/C/10 from rec[0/4/8], copy
 * owner anim id (ent->unk34 = src->unk34) and flags (ent->unk38 = src->unk38),
 * OR g_0604ebe4[ent->unk34] into ent->unk68, and (when g_060e0ddc != 0) fire
 * sound_queue_push(0xA0130CFF).
 *
 * Spawn a sub-entity via func_0220fece(task, handler 0x02239082); child->unkF =
 * src->unk44, child->unk13 = ent->unk59; ent->unk5A = ent->unk58 (seed
 * lifetime), ent->unk78 = child.  Finally task->unk0 = projectile_track_task
 * (0x0223144a).
 *
 * braf_resolve: 2313ac->0x23c74a(reserve ammo), 231408->0x215f6c
 * (sound_queue_push), 231414->0x20fece(spawn_obj_0x6c).
 * ptr_resolve: 231412 mova+add -> 0x02239082 (spawn handler),
 *              231438 mova+add -> 0x0223144a (next handler).
 */
extern u8   g_06097d58[];
extern s32  g_actor_slot_masks[];     /* per-anim flag word, idx anim id */
extern s32  g_sound_effects_enabled;       /* sound-enable flag */

extern s32   actor_consume_linked_effect_count(void *owner);                  /* reserve ammo (nonzero = fail) */
extern void  sound_queue_push(s32 req);                   /* sound_queue_push */
extern void *task_spawn_current_work6c(void *parent, void *handler);  /* spawn_obj_0x6c */
extern void  projectile_track_task(void *task);                   /* projectile_track_task */

void projectile_init_tracking_launch(void *task) {
    void *ent = TASK_PTR(task, work);
    void *src = MP(TASK_PTR(task, parent), 8);
    u8  *rec = &g_06097d58[((s32)M8(src, 0x34) << 8) + ((u8)M8(ent, 0x5F) << 4)];
    void *child;

    if (actor_consume_linked_effect_count(src) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(ent, 8)    = M32(rec, 0);
    M32(ent, 0xC)  = M32(rec, 4);
    M32(ent, 0x10) = M32(rec, 8);
    M8 (ent, 0x34) = M8(src, 0x34);
    M32(ent, 0x38) = M32(src, 0x38);
    M32(ent, 0x68) = M32(ent, 0x68) | g_actor_slot_masks[(s8)M8(ent, 0x34)];

    if (g_sound_effects_enabled != 0)
        sound_queue_push(0xA0130CFF);

    child = task_spawn_current_work6c(task, (void *)0x02239082);
    M8 (child, 0xF)  = M8(src, 0x44);
    M16(ent, 0x5A)   = M16(ent, 0x58);
    M8 (child, 0x13) = M8(ent, 0x59);
    MP (ent, 0x78)   = child;

    TASK_PTR(task, handler) = (void *)projectile_track_task;
}
/* fidelity: HIGH - ammo-reserve kill guard, record-seeded position, owner
   anim/flag copy, g_0604ebe4 attr OR, gated SFX, child spawn wiring and the
   installed next-handler (0x0223144a) all read from asm; every computed branch
   and data pointer confirmed by braf_resolve/ptr_resolve. This function was
   flagged pool-contaminated by find_entry (unit-grouper artifact) but the asm
   is a clean, complete function (prologue + rts epilogue). */

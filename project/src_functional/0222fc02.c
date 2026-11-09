/* spark_projectile_fire_task [0x0222fc02]: Existing map corroborated by source: state-0 fire (fixed-vel): copy owner anim/flags; unk68|=g_0604EBE4[anim]; unk62|=0x40; reserve ammo kill-on-fail; load pos; x2 unk64 if anim<3; post-clone init; SFX; (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0022fc02  spark_projectile_fire_task
 *
 * State-0 (init/fire) task for the spark-emitting arc projectile.  arg0 = task;
 * entity = task->unk8; owner = task->unk14->unk8.
 *
 * Copies the owner's anim id (unk34) and flags (unk38) onto the projectile, ORs
 * the per-anim flag word g_0604EBE4[anim] into unk68, sets bit 6 of unk62, and
 * reserves ammo via func_0223c74a(owner) - a non-zero return kills the task.
 * It then loads the start position from the animation record g_06097D58, doubles
 * unk64 for small anim ids (<3), runs post-clone init func_02238dde(), fires the
 * launch SFX (0xA01303FF) when sound is enabled, decrements lifetime unk58, and
 * installs the next state projectile_spark_arc_task (0x22fd54).
 *
 * Launch velocity is (unk14=0x12000, unk28=0x1000, unk18=-0x8000); if the owner
 * faces the other way (owner->unk44) the x components are negated and unk22=0x10.
 * Finally it draws one frame inside a matrix push/pop around g_06098878:
 * mtx_load_abc(); mtx_apply_vec_at0(record); mtx_apply_vec_at0(owner-facing vec
 * g_06087F3C[owner->unk44]); mtx_rotate_z(unk28); render func_022154fc(entity,
 * 0x40000, 0x20000, 2).
 *
 * Previously deferred: 8 computed branches. braf_resolve.py pins them:
 *   22fc56 -> 0x23c74a (reserve ammo)    22fca8 -> 0x238dde (post-clone init)
 *   22fcb8 -> 0x215f6c (sound_queue_push) 22fd06 -> 0x2149aa (mtx_load_abc)
 *   22fd0c -> 0x214a50 (mtx_apply_vec_at0) 22fd22 -> 0x214a50 (mtx_apply_vec_at0)
 *   22fd2c -> 0x213c4c (mtx_rotate_z)     22fd38 -> 0x2154fc (renderer)
 *
 * fidelity: HIGH - field copies, the ammo-kill guard, record load, velocity
 * constants, the facing flip, the matrix push/pop draw chain and the
 * next-state pointer (= 0x22fd54, mova+0x8C) all read from ROM asm; every
 * computed branch confirmed by braf_resolve.py.
 */
extern s32   g_actor_slot_masks_legacy[];      /* per-anim flag word (idx anim id) */
extern s32   g_sound_effects_enabled_legacy;        /* sound-enable flag */
extern s32   g_matrix_stack_cursor;        /* active matrix-stack pointer */

extern s32   actor_consume_linked_effect_count(void *owner);   /* reserve ammo (nonzero = fail) */
extern void  display_mark_channel_bits(void);          /* post-clone init */
extern void  sound_queue_push(u32 req);        /* sound_queue_push */
extern void  mtx_load_abc(void);          /* mtx_load_abc */
extern void  mtx_apply_vec_at0(void *vec);     /* mtx_apply_vec_at0 */
extern void  mtx_rotate_z(s32 angle);     /* mtx_rotate_z */
extern void  sprite_draw_billboard(void *ent, s32 sx, s32 sy, s32 flag); /* renderer */
extern void  projectile_spark_arc_task(void *task);    /* projectile_spark_arc_task */

#define G_RECORD_BASE     0x06097D58
#define G_FACING_06087F3C 0x06087F3C   /* owner-facing vectors, stride 12 */

void spark_projectile_fire_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *owner  = MP(TASK_PTR(task, parent), 8);
    s32   anim   = M8(owner, 0x34);
    void *record = (void *)(uintptr_t)(G_RECORD_BASE
                           + (anim << 8)
                           + (MU8(entity, 0x5F) << 4));

    M8(entity, 0x34) = anim;
    M32(entity, 0x38) = M32(owner, 0x38);
    M32(entity, 0x68) = M32(entity, 0x68) | g_actor_slot_masks_legacy[M8(entity, 0x34)];
    M16(entity, 0x62) = M16(entity, 0x62) | 0x40;

    if (actor_consume_linked_effect_count(owner) != 0) {
        TASK_S32(task, handler) = 0;
        return;
    }

    M32(entity, 0x08) = M32(record, 0x00);
    M32(entity, 0x0C) = M32(record, 0x04);
    M32(entity, 0x10) = M32(record, 0x08);
    if (M8(entity, 0x34) < 3)
        M32(entity, 0x64) = M32(entity, 0x64) << 1;

    display_mark_channel_bits();
    if (g_sound_effects_enabled_legacy != 0)
        sound_queue_push(0xA01303FF);

    M16(entity, 0x58) = M16(entity, 0x58) - 1;
    TASK_PTR(task, handler) = (void *)projectile_spark_arc_task;

    M16(entity, 0x28) = 0x1000;
    M32(entity, 0x14) = 0x12000;
    M32(entity, 0x18) = -0x8000;
    if (M8(owner, 0x44) != 0) {
        M32(entity, 0x14) = -M32(entity, 0x14);
        M16(entity, 0x28) = -M16(entity, 0x28);
        M16(entity, 0x22) = 0x10;
    }

    M16(entity, 0x30) = 0x17A;
    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0(record);
    mtx_apply_vec_at0((void *)(uintptr_t)(G_FACING_06087F3C
                  + (s8)(M8(owner, 0x44) * 12)));
    mtx_rotate_z((u16)M16(entity, 0x28));
    sprite_draw_billboard(entity, 0x40000, 0x20000, 2);
    g_matrix_stack_cursor -= 0x30;
    M16(entity, 0x30) = 0x179;
}

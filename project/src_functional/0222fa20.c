/* slide_projectile_fire_task [0x0222fa20]: Existing map corroborated by source: state-0 fire (table-vel sibling of 0x22fc02): copy owner anim/flags; unk68|=g_0604EBE4[anim]; reserve ammo (0x23c74a) kill-on-fail; load pos; x2 unk64 if anim<3; pos (src symbols_gen7.txt) */
#include "dh_work.h"
#include "dh.h"
/* 0x0022fa20  slide_projectile_fire_task
 *
 * State-0 (init/fire) task for the sliding spark projectile; sibling of
 * spark_projectile_fire_task (0x22fc02).  arg0 = task; entity = task->unk8;
 * owner = task->unk14->unk8.
 *
 * Copies the owner's anim id (unk34) and flags (unk38), ORs g_0604EBE4[anim]
 * into unk68, and reserves ammo via func_0223c74a(owner) (non-zero return kills
 * the task). It loads the start position from the animation record g_06097D58,
 * doubles unk64 for small anim ids (<3), runs post-clone init func_02238dde(),
 * fires the launch SFX when sound is enabled, decrements lifetime unk58 and
 * installs the next state spark_projectile_slide_task (0x22fb90).
 *
 * The launch velocity is read from the table g_06087F84 at index
 * ((unk62 & 63) - 11) * 4 + owner->unk44 * 2 (unk14 = table[idx], unk22 =
 * table[idx+1]). It then draws one frame inside a matrix push/pop around
 * g_06098878: mtx_load_abc(); mtx_apply_vec_at0(record); mtx_apply_vec_at0(
 * facing vec g_06087F24[(((unk63 & 63) - 11) ^ owner->unk44) * 12]);
 * render func_022154fc(entity, 0x40000, 0x20000, 2).
 *
 * Previously deferred: 7 computed branches. braf_resolve.py pins them:
 *   22fa6a -> 0x23c74a (reserve ammo)    22fa98 -> 0x238dde (post-clone init)
 *   22faa8 -> 0x215f6c (sound_queue_push) 22faf6 -> 0x2149aa (mtx_load_abc)
 *   22fafc -> 0x214a50 (mtx_apply_vec_at0) 22fb1e -> 0x214a50 (mtx_apply_vec_at0)
 *   22fb2a -> 0x2154fc (renderer)
 *
 * fidelity: HIGH - field copies, ammo-kill guard, record load, the table-driven
 * velocity index, the xor-indexed facing vector, the matrix push/pop draw chain
 * and the next-state pointer (= 0x22fb90, mova+0xD8) all read from ROM asm; every
 * computed branch confirmed by braf_resolve.py.
 */
extern s32   g_actor_slot_masks_legacy[];      /* per-anim flag word (idx anim id) */
extern s32   g_sound_effects_enabled_legacy;        /* sound-enable flag */
extern s32   g_matrix_stack_cursor;        /* active matrix-stack pointer */
extern s32   g_06087F84[];      /* launch-vector table */

extern s32   actor_consume_linked_effect_count(void *owner);   /* reserve ammo (nonzero = fail) */
extern void  display_mark_channel_bits(void);          /* post-clone init */
extern void  sound_queue_push(u32 req);        /* sound_queue_push */
extern void  mtx_load_abc(void);          /* mtx_load_abc */
extern void  mtx_apply_vec_at0(void *vec);     /* mtx_apply_vec_at0 */
extern void  sprite_draw_billboard(void *ent, s32 sx, s32 sy, s32 flag); /* renderer */
extern void  spark_projectile_slide_task(void *task);    /* spark_projectile_slide_task */

#define G_RECORD_BASE     0x06097D58
#define G_FACING_06087F24 0x06087F24   /* facing vectors, stride 12 */

void slide_projectile_fire_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *owner  = MP(TASK_PTR(task, parent), 8);
    s32   anim   = M8(owner, 0x34);
    void *record = (void *)(uintptr_t)(G_RECORD_BASE
                           + (anim << 8)
                           + (MU8(entity, 0x5F) << 4));
    s32   idx;

    M8(entity, 0x34) = anim;
    M32(entity, 0x38) = M32(owner, 0x38);
    M32(entity, 0x68) = M32(entity, 0x68) | g_actor_slot_masks_legacy[M8(entity, 0x34)];

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
    TASK_PTR(task, handler) = (void *)spark_projectile_slide_task;

    idx = ((M16(entity, 0x62) & 63) - 11) * 4 + (M8(owner, 0x44) << 1);
    M32(entity, 0x14) = g_06087F84[idx];
    M16(entity, 0x22) = g_06087F84[idx + 1];
    M16(entity, 0x30) = 0x17A;

    g_matrix_stack_cursor += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0(record);
    mtx_apply_vec_at0((void *)(uintptr_t)(G_FACING_06087F24
                  + (s8)((((M8(entity, 0x63) & 63) - 11) ^ M8(owner, 0x44)) * 12)));
    sprite_draw_billboard(entity, 0x40000, 0x20000, 2);
    g_matrix_stack_cursor -= 0x30;
    M16(entity, 0x30) = 0x179;
}

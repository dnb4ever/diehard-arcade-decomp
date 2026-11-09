/* projectile_init_attack_variant_a [0x022306c4]: Seeds owner-frame record, attack fields and handler 022308a0. */
#include "dh.h"
#include "dh_work.h"
/* 0x022306c4 - action/attack state setup task handler (arg0 = task).
 *   entity = arg0->unk8;  source = (arg0->unk14)->unk8  (the actor that owns the action).
 *
 * Seeds the frame from a 0x100-stride animation descriptor
 *   desc = g_06097d58 + ((s8)source->unk34 << 8) + (u8)entity->unk5F * 0x10
 * copies the move id (entity->unk34 = source->unk34) and unk38, ORs the move
 * layer/collision mask g_0604ebe4[unk34] into entity->unk68, and if the init hook
 * func_0223c7ba(source) returns nonzero it kills the task (arg0->unk0 = 0) and
 * returns.
 *
 * Otherwise: entity->unk62 |= 0x40; if unk34 < 3 double entity->unk64; copy the
 * descriptor position into entity->unk8/C/10; spawn the child effect
 * func_0220fece(arg0, func_02238df4); and, when g_06098670==1 && g_06098830>=2,
 * force g_06098830=2 and reset (g_0609882c)->unk8 (unkD=1, unkAE/B0/B2=0). Queues
 * sound 0xa01301ff when g_060e0ddc != 0. Installs func_022308a0 as the next task
 * handler (arg0->unk0).
 *
 * Pulls two param words from g_06087f84 indexed by
 *   idx = ((entity->unk62 & 0x3F) - 0x0B) * 4 + source->unk44 * 2
 * (unk14 = word[idx], unk22 = word[idx+1]), unk1C = unk14 >> 3, unk30 = 0x1a0,
 * then runs the matrix-draw cluster against the 0x30-byte scratch at g_06098878:
 *   func_022173f4(entity, g_06088064, 2);
 *   g_06098878 += 0x30; mtx_load_abc(); mtx_apply_vec_at0(desc);
 *   mtx_apply_vec_at0(g_06087f54 + (s8)(source->unk44 * 12));
 *   mtx_rotate_z((u16)entity->unk28); func_022154fc(entity, 0x40000, 0x30000, 2);
 *   g_06098878 -= 0x30; unk30 = 0x179.
 *
 * fidelity: HIGH on control flow and fields (hand-verified vs m2c_fn --asm +
 * braf_resolve, fn.py 0x2306c4); MED on the func_022173f4 draw-helper arg list
 * (undecompiled - same matrix-draw cluster flagged MED by effect_tumble_task).
 * Other callee ABIs taken from their decompiled sources. */
#define G16(a)  (*(s16 *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern s32  player_consume_linked_effect_count(void *source);                          /* init hook -> bool */
extern void task_spawn_current_work6c(void *parent, void *handler);            /* spawn_obj_0x6c */
extern void sound_queue_push(u32 cmd);                                /* sound_queue_push */
extern void collision_sweep_pose(void *entity, void *desc, s32 mode);     /* draw/anim setup (MED args) */
extern void mtx_load_abc(void);                                  /* mtx_load_abc */
extern void mtx_apply_vec_at0(void *obj);                             /* mtx_apply_vec_at0 */
extern void mtx_rotate_z(s32 angle);                             /* mtx_rotate_z */
extern void sprite_draw_billboard(u32 a, s32 b, s32 c, s32 d);
extern void display_pulse_decay_task(void *task);                             /* spawned child handler */
extern void projectile_tick_attack_variant_a(void *task);                             /* next task handler */

void projectile_init_attack_variant_a(void *arg0) {
    void *entity = TASK_PTR(arg0, work);
    void *source = MP(TASK_PTR(arg0, parent), 8);
    s32   move_id = M8(source, 0x34);
    void *desc = (void *)(uintptr_t)(0x06097d58 + (move_id << 8) + MU8(entity, 0x5F) * 0x10);
    s32   idx;

    M8(entity, 0x34)  = (s8)move_id;
    M32(entity, 0x38) = M32(source, 0x38);
    M32(entity, 0x68) |= G32(DH_ACTOR_SLOT_MASKS_ADDR + M8(entity, 0x34) * 4);

    if (player_consume_linked_effect_count(source) != 0) {
        TASK_PTR(arg0, handler) = 0;
        return;
    }

    M16(entity, 0x62) |= 0x40;
    if (M8(entity, 0x34) < 3)
        M32(entity, 0x64) *= 2;

    M32(entity, 0x08) = M32(desc, 0);
    M32(entity, 0x0C) = M32(desc, 4);
    M32(entity, 0x10) = M32(desc, 8);

    task_spawn_current_work6c(arg0, (void *)display_pulse_decay_task);

    if (G32(DH_PLAY_SESSION_MODE_ADDR) == 1 && G32(DH_VIEW_REQUEST_PRIORITY_ADDR) >= 2) {
        void *sub = MP((void *)(uintptr_t)(u32)G32(DH_VIEW_TASK_ADDR), 8);
        G32(DH_VIEW_REQUEST_PRIORITY_ADDR) = 2;
        MU8((void *)(uintptr_t)(u32)G32(DH_VIEW_TASK_ADDR), 0xD) = 1;
        M16(sub, 0xAE) = 0;
        M16(sub, 0xB0) = 0;
        M16(sub, 0xB2) = 0;
    }

    if (G32(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
        sound_queue_push(0xa01301ff);

    TASK_PTR(arg0, handler) = (void *)projectile_tick_attack_variant_a;

    collision_sweep_pose(entity, (void *)(uintptr_t)0x06088064, 2);

    idx = ((M16(entity, 0x62) & 0x3F) - 0x0B) * 4 + M8(source, 0x44) * 2;
    M32(entity, 0x14) = G32(0x06087f84 + idx * 4);
    M16(entity, 0x22) = (s16)G32(0x06087f84 + (idx + 1) * 4);
    M32(entity, 0x1C) = M32(entity, 0x14) >> 3;
    M16(entity, 0x30) = 0x1a0;

    G32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;
    mtx_load_abc();
    mtx_apply_vec_at0(desc);
    mtx_apply_vec_at0((void *)(uintptr_t)(0x06087f54 + (s8)(M8(source, 0x44) * 12)));
    mtx_rotate_z((u16)M16(entity, 0x28));
    sprite_draw_billboard((u32)(uintptr_t)entity, 0x40000, 0x30000, 2);
    G32(DH_MATRIX_STACK_CURSOR_ADDR) -= 0x30;

    M16(entity, 0x30) = 0x179;
}

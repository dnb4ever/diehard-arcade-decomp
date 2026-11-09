/* camera_tick_sound_timeline [0x022203ce]: Frames 62/122/182/360/422/425/430 emit cues during camera motion. */
#include "dh_work.h"
#include "dh.h"
/* 0x022203ce  scripted camera + SFX-cue timeline task.
 * task (r4): +0x08 -> work struct, +0x10 = state byte.
 * work: +0x08/0x0C/0x10 camera position, +0x14/0x18/0x1C extra, +0x24 angle, +0x26 angle2,
 *       +0x36/0x37 flags, +0x60 frame counter.
 *   state 0: seed camera (unk8=0x200000, unkC=0x80000, unk10=0x1C0000, unk1C=-0x1000, unk24=0x800,
 *            unk36=unk37=1, unk60=0), fire the default sound cue, advance; return.
 *   state 1: ++unk60; at specific frames (62/122/182/360/422/425/430) fire paired sound cues via
 *            sound_queue_push (all gated by the SFX-enable flag g_060E0DDC); frame 430 also advances.
 *            All frames then run the common tail.
 *   state 2+/common: when g_06098794 has bits 0x10|0x20 set and g_060987BC bit0 set, install the
 *            continuation handler func_0222078c via func_0220ff54(task, ...); step unk26 += 30,
 *            run mtx_push_rotate_apply over &unk14/&unk24 (with unk24 briefly biased by -0x800), add the
 *            per-frame camera deltas g_06098B1C[0..2] into unk8/unkC/unk10, tail-call func_0221b330.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. jsr @r12 -> 0x215f6c; handler 0x22078c (mova+add);
 * bsrf -> 0x20ff54, 0x21080a; braf (tail) -> 0x21b330. */

extern u8 g_06098794[];   /* u16 status bits */
extern u8 g_frame_tick[];   /* s32 flag (bit0) */
extern u8 g_sound_effects_enabled_legacy[];   /* s32 SFX-enable flag */
extern u8 g_transformed_vector_legacy[];   /* s32[3] per-frame camera deltas */
extern void sound_queue_push(u32 cmd);
extern void task_spawn_current_work40(void *task, void (*handler)(void *));
extern void mtx_push_rotate_apply(void *a, void *b);
extern void model_draw_composite_parts(void *work);
extern void effect_tick_ballistic_sprite(void *task);

static void fire2(u32 a, u32 b) {
    if (M32(g_sound_effects_enabled_legacy, 0) != 0)
        sound_queue_push(a);
    if (M32(g_sound_effects_enabled_legacy, 0) != 0)
        sound_queue_push(b);
}

void camera_tick_sound_timeline(void *task) {
    void *work = TASK_PTR(task, work);

    switch (TASK_U8(task, state)) {
    case 0:
        M32(work, 0x08) = 0x200000;
        M32(work, 0x0C) = 0x80000;
        M32(work, 0x10) = 0x1C0000;
        M32(work, 0x14) = 0;
        M32(work, 0x18) = 0;
        M32(work, 0x1C) = -0x1000;
        M16(work, 0x24) = 0x800;
        if (M32(g_sound_effects_enabled_legacy, 0) != 0)
            sound_queue_push(DH_SOUND_EFFECT_VARIANT_COMMAND);
        MU8(work, 0x37) = 1;
        MU8(work, 0x36) = 1;
        M32(work, 0x60) = 0;
        TASK_U8(task, state)++;
        return;
    case 1: {
        s32 f = M32(work, 0x60) + 1;
        M32(work, 0x60) = f;
        switch (f) {
        case 62:  fire2(DH_SOUND_CAMERA_TIMELINE_A_COMMAND, DH_SOUND_CAMERA_TIMELINE_B_COMMAND); break;
        case 122: fire2(DH_SOUND_CAMERA_TIMELINE_C_COMMAND, DH_SOUND_EFFECT_VARIANT_COMMAND); break;
        case 182: fire2(DH_SOUND_CAMERA_TIMELINE_A_COMMAND, DH_SOUND_CAMERA_TIMELINE_B_COMMAND); break;
        case 360: fire2(DH_SOUND_CAMERA_TIMELINE_C_COMMAND, DH_SOUND_PROP_LAUNCH_COMMAND); break;
        case 422: fire2(DH_SOUND_CAMERA_TIMELINE_D_COMMAND, DH_SOUND_CAMERA_TIMELINE_B_COMMAND); break;
        case 425: fire2(DH_SOUND_CAMERA_TIMELINE_C_COMMAND, DH_SOUND_EFFECT_VARIANT_COMMAND); break;
        case 430:
            if (M32(g_sound_effects_enabled_legacy, 0) != 0)
                sound_queue_push(DH_SOUND_CAMERA_TIMELINE_A_COMMAND);
            TASK_U8(task, state)++;
            break;
        default:
            break;
        }
        break;
    }
    default:
        break;
    }

    /* common tail (states 1 and 2+) */
    if ((MU16(g_06098794, 0) & 0x30) == 0x30 && (M32(g_frame_tick, 0) & 1))
        task_spawn_current_work40(task, effect_tick_ballistic_sprite);
    M16(work, 0x26) = (s16)(M16(work, 0x26) + 30);
    M16(work, 0x24) = (s16)(M16(work, 0x24) - 0x800);
    mtx_push_rotate_apply((u8 *)work + 0x14, (u8 *)work + 0x24);
    M16(work, 0x24) = (s16)(M16(work, 0x24) + 0x800);
    M32(work, 0x08) += M32(g_transformed_vector_legacy, 0);
    M32(work, 0x0C) += M32(g_transformed_vector_legacy, 4);
    M32(work, 0x10) += M32(g_transformed_vector_legacy, 8);
    model_draw_composite_parts(work);
}

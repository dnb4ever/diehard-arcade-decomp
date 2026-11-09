#include "dh_work.h"
/* 0x0221f2bc (scene_intro_bootstrap_sm) - a scene bootstrap state machine, sibling
 * of scene_bootstrap_sm (0x022027e4). FUNCTIONAL decomp, band 0x21xxxx.
 *
 * ABI: task_handler(task). task = r4; entity = task->unk8 (r12).
 * Config record = 0x060752B0 + (s8)(scene_dir->unk29 * 12), where
 * scene_dir = *0x0609866C (same record base as scene_bootstrap_dispatch_52b0).
 *
 * Pre-switch: on a 2-player panel (g_ctrl cooked word @0x06098798 & 0x30) while in
 * state 2, re-read the panel and let START (panel action byte == 2, either seat on
 * a 2P panel, or P1 on a 1P panel) skip straight to the commit state (state 3,
 * clearing entity->unk3C).
 *
 * States (task->unk10, byte):
 *   0: func_02217d6c(task) (init step); -> state 1.
 *   1: func_02217f04(task); install scene object handler 0x060136f6 into
 *      *0x0609882C (unk0) + clear its unk10; seed the display record (*0x06098F08)
 *      recoil triple unk114/116/118 = -0xFF; spawn a recoil channel object
 *      (spawn_obj_0x6c -> func_022100f8) seeded with rec->unk8 into obj8->unk50,
 *      dispatch its sound cue (sound_cue_dispatch); func_0222a8d8(); spawn the
 *      scripted camera/SFX timeline task (func_022203ce) and a scene handler
 *      (func_0221fe00, stored into entity->unk64) whose obj8 gets pos 0x200000/0/0x200000;
 *      -> state 2.
 *   2: gated on g_06098664==1: spawn another recoil channel (func_022101ce) seeded
 *      with rec->unkA, emit a gated sound cue (sound_emit_gated, id=(0x80-|rec->unkA|)>>1);
 *      -> state 3.
 *   3: once the scene exit object is done (!(entity->unk3C & 2)): mark the scene
 *      director (task->unk14->unk8->unk3C |= 1), enable the VDP2 display
 *      (TVMD 0x25F80000 = displayrec->unk0 & 0x7FFF), tear down the scene-ready
 *      globals (0x06099578, 0x060BACAC/B0/B4 bit0, 0x060986B8, 0x0609861C, g_06098664),
 *      queue sound 0xA00003FF, and tail-call screen_fade_sequencer(task->unkE + 0xFF).
 *
 * Branches braf-resolved to real TARGETs; pointers ptr_resolved (mova+add):
 * 0x060136f6, 0x060140f8, 0x060243ce, 0x06023e00, 0x060141ce.
 * m2c correction: func_02215f6c(0xA00003FF, 0x060BACB4) is sound_queue_push(0xA00003FF)
 * - the 0x060BACB4 is a stale r5 (the slot cleared just before), NOT an argument.
 * No tst #imm8>=0x80 sign-ext traps (imm are 0x30/0x04/0x02 only). Bounded
 * 0x21f2bc..0x21f532 (single rts). fidelity: HIGH. */
#include "dh_band21.h"

#define READ_PANEL(buf) \
    ((void (*)(u32))(uintptr_t)(u32)MEM32(0x06000660))((u32)(buf))

extern void scene_reload_graphics(u32 task);                /* bootstrap init step        */
extern void scene_reset_stage(u32 task);                /* bootstrap init step        */
extern u32  task_spawn_current_work6c(u32 task, u32 handler);   /* spawn_obj_0x6c             */
extern u32  task_spawn_child_work88(u32 task, u32 handler);   /* spawn_obj_a                */
extern void player_refresh_ready_mask(void);                    /* 2P-select helper           */
extern void audio_queue_scene_sound(u32 obj);                 /* sound_cue_dispatch         */
extern void audio_queue_scene_gated_id(s32 id);                  /* sound_emit_gated           */
extern void sound_queue_push_entry(s32 cmd);                 /* sound_queue_push           */
extern void task_cancel_below_depth(s32 arg);                 /* screen_fade_sequencer (tail)*/

void scene_intro_bootstrap_sm(u32 task)
{
    u32 entity    = MEM32(task + 8);
    u32 scene_dir = MEM32(DH_SCENE_DESCRIPTOR_ADDR);
    u32 rec       = 0x060752B0 + (s8)((s8)SCENE_MEM8(scene_dir, angle_variant) * 12);

    if ((MEMU16(0x06098798) & 0x30) && MEM8(task + 0x10) == 2) {
        READ_PANEL(0x060987D8);
        if (MEM8(0x060987E8) & 4) {
            u16 f = MEMU16(0x06098798);
            if (((f & 0x10) && MEM8(0x060987DB) == 2) ||
                ((f & 0x20) && MEM8(0x060987DF) == 2)) {
                MEM32(entity + 0x3C) = 0;
                MEM8(task + 0x10) = 3;
            }
        } else if ((MEMU16(0x06098798) & 0x30) && MEM8(0x060987DB) == 2) {
            MEM32(entity + 0x3C) = 0;
            MEM8(task + 0x10) = 3;
        }
    }

    switch (MEM8(task + 0x10)) {
    case 0:
        scene_reload_graphics(task);
        MEM8(task + 0x10) = (u8)(MEM8(task + 0x10) + 1);
        break;

    case 1: {
        scene_reset_stage(task);
        u32 sobj = MEM32(DH_VIEW_TASK_ADDR);
        u32 disp = MEM32(DH_VDP2_REGISTER_SHADOW_ADDR);
        u32 o1, o2;

        MEM32(sobj + 0) = 0x060136f6;   /* install scene object handler */
        MEM8(sobj + 0x10) = 0;

        MEM16(disp + 0x114) = -0xFF;
        MEM16(disp + 0x116) = -0xFF;
        MEM16(disp + 0x118) = -0xFF;

        o1 = task_spawn_current_work6c(task, 0x060140f8);        /* recoil channel */
        MEM32(MEM32(o1 + 8) + 0x50) = MEM16(rec + 8);
        audio_queue_scene_sound(o1);
        player_refresh_ready_mask();

        task_spawn_child_work88(task, 0x060243ce);             /* camera/SFX timeline */
        o2 = task_spawn_child_work88(task, 0x06023e00);        /* scene handler */
        MEM32(entity + 0x64) = o2;
        {
            u32 e2 = MEM32(o2 + 8);
            MEM32(e2 + 8)   = 0x200000;
            MEM32(e2 + 0xC) = 0;
            MEM32(e2 + 0x10) = 0x200000;
        }
        MEM8(task + 0x10) = 2;
        break;
    }

    case 2:
        if (MEM32(0x06098664) == 1) {
            u32 o = task_spawn_current_work6c(task, 0x060141ce); /* recoil channel */
            s16 a = MEM16(rec + 0xA);
            s32 abs_a = (a < 0) ? -a : a;
            MEM32(MEM32(o + 8) + 0x50) = a;
            audio_queue_scene_gated_id((0x80 - abs_a) >> 1);
            MEM8(task + 0x10) = 3;
        }
        break;

    case 3:
        if (!(MEM32(entity + 0x3C) & 2)) {
            MEM32(MEM32(MEM32(task + 0x14) + 8) + 0x3C) |= 1;
            MEM16(VDP2_TVMD_ADDR) = MEM16(MEM32(DH_VDP2_REGISTER_SHADOW_ADDR)) & 0x7FFF; /* VDP2 TVMD */
            MEM32(0x06099578) = 0;
            MEM32(0x060BACAC) &= ~1;
            MEM32(0x060BACB0) &= ~1;
            MEM32(0x060BACB4) &= ~1;
            sound_queue_push_entry(DH_SOUND_TRANSITION_DONE_COMMAND);               /* sound_queue_push */
            MEM32(0x060986B8) = 0;
            MEM8(0x0609861C) = 0;
            MEM32(0x06098664) = 0;
            task_cancel_below_depth((s8)MEM8(task + 0xE) + 0xFF); /* screen_fade_sequencer */
        }
        break;
    }
}

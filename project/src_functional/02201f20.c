#include "dh_work.h"
/* scene_display_setup_task [0x02201f20]: Three task states configure VDP2, count down and notify the parent scene. */
#include "dh.h"
/* 0x0201f20 (vdp2_update_b / scene display-mode setup sequencer) - task handler (arg0 = task,
 * task->unk8 = entity). A 3-state sequencer on task->unk10:
 *  - state 0: full VDP2 re-init. func_02216154(-1); program the VDP2 register shadow block
 *    *g_06098F08 (unkE = (unkE & ~0x300)|0x300, unk10=-1, unk14=0x454FFFFF, unk18=-1,
 *    unk1C=0x0137FFFF, unk0 bits (&~0x30|0x10, &~7|1, finally &0x7FFF|0x8000), unkE0 (&~0xF|0xF),
 *    unkF0=5, unkF2/F4/F6=0, unkF8=0x603, unkFA=0x700, unkFC=4); func_02222d3c(7); call the HWRAM
 *    hook *g_06090294(); func_02214d26() (vdp1_draw_kick). Then copy g_0609866C->unk10 into
 *    *g_0609882C and run func_02227a84 on it, func_0223c95c(), func_02216c38(g_0609866C->unk20,
 *    ->unk24), func_02207604(task, g_0609866C). Seed entity->unk50 = 0x96, g_06099578 = 1,
 *    task->unk10 = 1 and fall into state 1.
 *  - state 1: count entity->unk50 down; when it expires (or input g_06098794[6] & 3 is pressed),
 *    clear g_06099578, turn the display on via VDP2 TVMD (reg 0x25F80000 = shadow unk0 & 0x7FFF)
 *    and advance task->unk10 = 2.
 *  - state 2: OR bit0 into task->unk14->unk8->unk3C, then tail func_0220fdc8(0).
 * fidelity: HIGH - VDP2 shadow fields word except unk10..unk1C long; entity->unk50 long,
 * task->unk10 byte, input word; TVMD is a 16-bit HW register; computed branches resolved via
 * braf_rewrite.py (incl. the braf tail to func_0220fdc8), the HWRAM hook *g_06090294 is an
 * indirect jsr. */
extern void video_reset_buffers(s32 a);
extern void hud_setup_small_font(s32 a);
extern void vdp1_trigger_draw(void);          /* vdp1_draw_kick */
extern void actor_reset_slot_registry(void *p);
extern void effect_pool_reset_scene(void);
extern void stage_initialize_grids(s32 a, s32 b);
extern void scene_load_descriptor_layer(void *task, void *p);
extern void task_cancel_below_depth(s32 a);
extern void (*g_06090294)(void);          /* HWRAM VDP2 hook */
extern void *g_vdp2_register_shadow;                  /* -> VDP2 register shadow block */
extern void *g_scene_descriptor;                  /* -> scene descriptor */
extern void *g_view_task_legacy;                  /* -> secondary descriptor */
extern u8   g_06098794[];                 /* input state block */
extern s32  g_06099578;
extern volatile s16 reg_vdp2_tvmd;         /* VDP2 TVMD */

void scene_display_setup_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *vdp;
    void *p;

    switch (TASK_S8(task, state)) {
    case DH_DISPLAY_CONFIGURE:
        video_reset_buffers(-1);
        vdp = g_vdp2_register_shadow;
        M16(vdp, 0xE) = (s16)((M16(vdp, 0xE) & ~0x300) | 0x300);
        M32(vdp, 0x10) = -1;
        M32(vdp, 0x14) = 0x454FFFFF;
        M32(vdp, 0x18) = -1;
        M32(vdp, 0x1C) = 0x0137FFFF;
        hud_setup_small_font(7);
        g_06090294();
        vdp1_trigger_draw();
        vdp = g_vdp2_register_shadow;
        M16(vdp, 0) = (s16)((M16(vdp, 0) & ~0x30) | 0x10);
        M16(vdp, 0) = (s16)((M16(vdp, 0) & ~7) | 1);
        M16(vdp, 0xE0) = (s16)((M16(vdp, 0xE0) & ~0xF) | 0xF);
        M16(vdp, 0xF8) = 0x603;
        M16(vdp, 0xFA) = 0x700;
        M16(vdp, 0xFC) = 4;
        M16(vdp, 0xF0) = 5;
        M16(vdp, 0xF2) = 0;
        M16(vdp, 0xF4) = 0;
        M16(vdp, 0xF6) = 0;
        M16(vdp, 0) = (s16)((M16(vdp, 0) & 0x7FFF) | (s16)0x8000);
        p = g_view_task_legacy;
        M32(p, 0) = SCENE_M32(g_scene_descriptor, camera_definition);
        M8(p, 0x10) = 0;
        actor_reset_slot_registry(p);
        effect_pool_reset_scene();
        stage_initialize_grids(SCENE_M32(g_scene_descriptor, stage_index), SCENE_M32(g_scene_descriptor, view_radius));
        scene_load_descriptor_layer(task, g_scene_descriptor);
        M32(entity, 0x50) = 0x96;
        g_06099578 = 1;
        TASK_S8(task, state) = DH_DISPLAY_WAIT;
        /* fall through */
    case DH_DISPLAY_WAIT: {
        s32 c = M32(entity, 0x50) - 1;
        M32(entity, 0x50) = c;
        if (c + 1 == 0 || (MU16(g_06098794, 6) & 3)) {
            g_06099578 = 0;
            reg_vdp2_tvmd = (s16)(M16(g_vdp2_register_shadow, 0) & 0x7FFF);
            TASK_S8(task, state) = DH_DISPLAY_NOTIFY_PARENT;
        }
        return;
    }
    case DH_DISPLAY_NOTIFY_PARENT:
        p = MP(TASK_PTR(task, parent), 8);
        M32(p, 0x3C) |= 1;
        task_cancel_below_depth(0);
        return;
    default:
        return;
    }
}

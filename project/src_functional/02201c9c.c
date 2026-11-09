#include "dh_work.h"
/* scene_palette_fade_task [0x02201c9c]: Advances palette fade and enables the display/player session on completion. */
#include "dh.h"
/* 0x0201c9c (vdp2_update_a / screen fade sequencer) - task handler (task->unk8 = entity). When the
 * entity's "dirty" flag unk3C & 1 is set, advances a CRAM/palette fade:
 *  - clears unk3C bit 0; g_06099578 = 0; g_06098812 = g_0609880E = 1; measures elapsed frames via
 *    the BIOS timer (*g_06000640) and feeds func_0220a962(now - g_060987F4), then latches
 *    g_060987F4 = *g_06000640(); clears g_06098690/94/570;
 *  - picks the fade level: if task->unkD < 0 force level 2 (base = g_06074000 + 2*0x50); else ramp
 *    g_06098668 up by one each frame while < 10; once >= 10, if config bit 5 is set finalise the
 *    level from the region table (g_060511DC[(g_06000248>>12)&0xF]==1 ? 1 : 2), else hand the palette
 *    to the BIOS fade routine *0x0600060C(g_060987C8, &g_0609868E, 2, g_06074000) and return. For the
 *    non-BIOS paths, latch g_0609868E = base->unk2A, point g_0609866C at the current palette base and
 *    apply it via func_0220fece(task, g_0609866C->unk14, step, g_06074000);
 *  - common tail: when config bit 5 is set and the fade level is nonzero, for each active plane
 *    (g_0609880C == 1, then g_06098810 == 1) mark the scene ready: entity->unk3C |= 1, play sound
 *    sound_queue_push(0xA00001FF), func_0220fdc8(0), g_06099578 = 0, enable VDP2 display
 *    (TVMD reg 0x25F80000 = shadow g_06098F08->unk0 & 0x7FFF), clear g_06098678->unk8->unk40/unk44,
 *    set g_06098670 = 1.
 * fidelity: HIGH - widths from the loads (fade level/flags/pos globals long, g_06098812/0E/0C/10
 * word, g_0609868E byte, unk3C long, unk14/unk40/unk44 long, base->unk2A byte); func_0220fdc8 and
 * the BIOS timer/fade are indirect through fixed HWRAM pointers; func_0220fdc8 target resolved via
 * ptr_resolve.py (mova 0x201cb4 + 0xE114 -> 0x0220fdc8); computed calls via braf_rewrite.py. */
extern s32  (*g_06000640)(void);                       /* BIOS frame/timer read */
extern void (*g_0600060C)(s32 a, void *b, s32 c, void *d);  /* BIOS palette fade */
extern void resident_add_session_value(s32 a);
extern void task_cancel_below_depth(s32 a);
extern void task_spawn_current_work6c(void *task, s32 a, s32 step, void *base);
extern void sound_queue_push(s32 cmd);                    /* sound_queue_push */
extern u8   g_06074000[];      /* palette/fade source table (0x50 stride) */
extern void *g_scene_descriptor;       /* current palette base */
extern void *g_vdp2_register_shadow;       /* -> VDP2 register shadow block */
extern void *g_06098678;       /* -> scene plane descriptor */
extern s32  g_060511DC[];      /* region stride table */
extern u32  g_06000798, g_06000248;
extern s32  g_scene_index, g_06099578, g_060987F4, g_06098690, g_06098694, g_06098570;
extern s32  g_play_session_mode, g_060987C8;
extern s16  g_06098812, g_0609880E, g_0609880C, g_06098810;
extern s8   g_0609868E;
extern volatile s16 reg_vdp2_tvmd;  /* VDP2 TVMD */

static void fade_scene_ready(void *entity) {
    M32(entity, 0x3C) |= 1;
    sound_queue_push(DH_SOUND_SCENE_END_COMMAND);
    task_cancel_below_depth(0);
    g_06099578 = 0;
    reg_vdp2_tvmd = (s16)(M16(g_vdp2_register_shadow, 0) & 0x7FFF);
    M32(MP(g_06098678, 8), 0x40) = 0;
    M32(MP(g_06098678, 8), 0x44) = 0;
    g_play_session_mode = 1;
}

void scene_palette_fade_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *base;

    if (M32(entity, 0x3C) & 1) {
        s32 now;
        task_cancel_below_depth(0);
        M32(entity, 0x3C) &= ~1;
        g_06099578 = 0;
        g_06098812 = 1;
        g_0609880E = 1;
        now = g_06000640();
        resident_add_session_value(now - g_060987F4);
        g_060987F4 = g_06000640();
        g_06098690 = 0;
        g_06098694 = 0;
        g_06098570 = 0;
        if (M8(task, 0xD) < 0) {
            g_scene_index = DH_SCENE_PLAY_ENTRY;
            base = g_06074000 + 2 * 0x50;
            g_scene_descriptor = base;
            g_0609868E = M8(base, 0x2A);
            task_spawn_current_work6c(task, M32(g_scene_descriptor, 0x14), 2 * 0x50, g_06074000);
        } else if (g_scene_index < 0xA) {
            s32 lvl = g_scene_index + 1;
            g_scene_index = lvl;
            base = g_06074000 + (s16)(lvl * 0x50);
            g_scene_descriptor = base;
            g_0609868E = M8(base, 0x2A);
            task_spawn_current_work6c(task, M32(g_scene_descriptor, 0x14), 2, g_06074000);
        } else if (((g_06000798 >> 5) & 1) == 1) {
            g_scene_index = (g_060511DC[(g_06000248 >> 0xC) & 0xF] != 1) ? 2 : 1;
            base = g_06074000 + (s16)(g_scene_index * 0x50);
            g_scene_descriptor = base;
            g_0609868E = M8(base, 0x2A);
            task_spawn_current_work6c(task, M32(g_scene_descriptor, 0x14), 2, g_06074000);
        } else {
            g_0600060C(g_060987C8, &g_0609868E, 2, g_06074000);
            return;
        }
    }
    if (((g_06000798 >> 5) & 1) == 1 && g_scene_index != DH_SCENE_SELECTOR) {
        if (g_0609880C == 1)
            fade_scene_ready(entity);
        if (g_06098810 == 1)
            fade_scene_ready(entity);
    }
}

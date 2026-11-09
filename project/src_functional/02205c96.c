/* Round 4: Original directional gates store scene index only when a direction is accepted; remove extra unchanged writes from inactive gates. Evidence: work/decomp4/05c96.asm. */
/* scene_select_index_task_entry [0x02205c96]: Builds the scene selector display and commits the input-selected scene index. */
#include "dh_work.h"
#include "dh.h"
/* 0x02205c96 (scene0_attract_select) - the SCENE-0 handler, i.e. the task installed as
 * scene-0's "off14" (descriptor 0x06072660, off14 = rt 0x06009c96). This is the scene the
 * director falls back to after scene 81 (scene81 descriptor next44 = 0 -> scene 0) and the
 * scene scene_bootstrap parks on in test/auto-start boots. ABI (task): task = r4(arg0);
 * entity/director-object reached via task->unk14->unk8. Per-frame 3-state machine on
 * task->unk10 (byte @+0x10), a cmp/eq chain rendered faithfully as a switch (states 0,1,2;
 * NO jump table - the dispatch is cmp/eq #0/#1/#2 with an "else" to the epilogue).
 *
 *   state 0 (cart 0x205d2c): build the display shadow (*0x06098F08) EXACTLY like
 *            scene81_game_sm/scene_credit_join_sm state 0 (video_state_init(-1),
 *            large-font setup 7, vdp1_draw_kick, upload the scene's tile groups from
 *            *0x0609866C, entity-system reset), seed the aux object 0x0609882C from
 *            scenedesc->unk10, print the scene header (fmt 0x06050DD4), FORCE the director
 *            index g_06098668 = 2, and advance to state 1 (falls through into it).
 *   state 1 (cart 0x205e74, THE ATTRACT / SCENE-SELECT LOOP): poll the two 0x18-byte panel
 *            input records at 0x06098734 (P1) and 0x0609874C (P2); each has unk4 = pressed-
 *            edge bits, unk0 = held bits, unkC = hold timer (>10 => auto-repeat). The UP bit
 *            (0x80) increments g_06098668 (wrap 81->0), the DOWN bit (0x40) decrements it
 *            (wrap 0->81). It then prints the current index line (fmt 0x06050DE4 + the index
 *            and the per-index word g_0609031C[idx]). An ACTION press (any of bits 0x7 on
 *            either record) advances to state 2. >>> This is the game's scene-check / attract
 *            index pivot: with no input it sits on index 2 (the first stage), which is where
 *            the director's own scene-0 selector (cur[43] = -1 -> idx 2) also points.
 *   state 2 (cart 0x205faa, COMMIT): set the scene-director advance bit
 *            (task->unk14->unk8->unk3c |= 1) so the director leaves scene 0 and builds
 *            g_06098668, blank the VDP2 display (0x25F80000 = disp->unk0 & 0x7FFF), clear the
 *            start/EDSR gate 0x06099578, tail-call kill_objects_above_id(0) (0x0220fdc8).
 *   default: just the register-restore epilogue (no body).
 *
 * Branches resolved via braf_resolve (bsrf PIC calls, all 11 real calls): video_state_init
 * 0x216154, vdp2_setup_largefont 0x222e44, vdp1_draw_kick 0x214d26, func_02216b14, func_02209100,
 * upload_texture_groups 0x2150d4, func_02215228, func_02223ac0, entity_system_reset 0x227a84,
 * func_0223c95c, func_02216c38; tail braf -> kill_objects_above_id 0x20fdc8. The two
 * "TABLE/COMPUTED" bsrf flagged by braf_resolve (0x205e90/0x205eec) are trailing literal-pool
 * words misdecoded as code, not calls. fidelity: HIGH on states 0/2 and the state-1 index
 * scroll (hand-traced vs m2c_fn --asm); the state-1 header-print thunk (*0x060902A0) is a
 * variadic UI print - the three observed stack pushes (fmt, g_0609031C[idx], idx) are modeled;
 * its r4..r7 are caller-indeterminate and not relied on. */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))
#define DISP     (*(void **)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR)

#define CTRL1    DH_PLAYER_INPUT_RECORDS_ADDR   /* P1 panel input record (0x18 bytes) */
#define CTRL2    0x0609874C   /* P2 panel input record (0x18 bytes) */

extern void  video_state_init(s32 mode);               /* 0x02216154 */
extern void  vdp2_setup_largefont(s32 arg);            /* 0x02222e44 */
extern void  vdp1_draw_kick(void);                     /* 0x02214d26 */
extern void  vdp2_apply_sprite_commands(void *arg);                 /* 0x02216b14 */
extern void  background_emit_grayscale_ramp(void *task);                /* 0x02209100 */
extern void  upload_texture_groups(s32 group);         /* 0x022150d4 */
extern void  vdp2_upload_pattern_groups(s32 arg);                   /* 0x02215228 */
extern void  model_load_scene_records(s32 arg);                   /* 0x02223ac0 */
extern void  entity_system_reset(void);                /* 0x02227a84 */
extern void  effect_pool_reset_scene(void);                      /* 0x0223c95c */
extern void  stage_initialize_grids(s32 a, s32 b);              /* 0x02216c38 */
extern void  kill_objects_above_id(u8 id);             /* 0x0220fdc8 */

/* one 0x18-byte panel record: edge bits (unk4), held bits (unk0), hold timer (unkC) */
static int panel_dir(const void *rec, u32 bit) {
    return (MU16(rec, 0x4) & bit) ||
           ((MU16(rec, 0x0) & bit) && M32(rec, 0xC) > 10);
}

void scene0_attract_select(void *task) {
    void *disp;
    void *aux;
    void *dir;
    s32   g;

    switch (TASK_U8(task, state)) {
    case 0:
        R32A(0x06099578) = 0;
        video_state_init(-1);
        disp = DISP;
        MU16(disp, 0xE) = (u16)((MU16(disp, 0xE) & 0xFCFF) | 0x0300);
        MU16(disp, 0xE) = (u16)((MU16(disp, 0xE) & 0xFF00) | 0x23);
        M32(disp, 0x10) = -1;
        M32(disp, 0x14) = 0x454FFFFF;
        M32(disp, 0x18) = -1;
        M32(disp, 0x1C) = 0x0137FFFF;
        vdp2_setup_largefont(7);
        ((void (*)(s32))PA(0x06090294))(7);
        vdp1_draw_kick();
        disp = DISP;
        MU16(disp, 0x0) = (u16)((MU16(disp, 0x0) & ~0x30) | 0x10);
        MU16(disp, 0x0) = (u16)((MU16(disp, 0x0) & ~0x7) | 0x3);
        vdp2_apply_sprite_commands((void *)0x060506DC);
        disp = DISP;
        MU16(disp, 0xE0) = (u16)((MU16(disp, 0xE0) & ~0xF) | 0xF);
        MU16(disp, 0xE0) = (u16)(MU16(disp, 0xE0) & ~0x10);
        MU16(disp, 0xF8) = 0x0603;
        MU16(disp, 0xFA) = 0x0700;
        MU16(disp, 0xFC) = 4;
        MU16(disp, 0xF0) = 5;
        MU16(disp, 0xF2) = 0;
        MU16(disp, 0xF4) = 0;
        MU16(disp, 0xF6) = 0;
        background_emit_grayscale_ramp(task);
        MU16(DISP, 0x0) = (u16)((MU16(DISP, 0x0) & 0x7FFF) | 0x8000);
        R32A(0x06099578) = 1;
        upload_texture_groups(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x0));
        vdp2_upload_pattern_groups(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x4));
        model_load_scene_records(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x8));
        aux = PA(DH_VIEW_TASK_ADDR);
        M32(aux, 0x0) = M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x10);
        MU8(aux, 0x10) = 0;
        R32A(DH_UI_CURSOR_COLUMN_ADDR) = 0x10;
        R32A(DH_UI_CURSOR_ROW_ADDR) = 4;
        ((void (*)(void *, const void *))PA(0x060902A0))(aux, (const void *)0x06050DD4);
        entity_system_reset();
        effect_pool_reset_scene();
        stage_initialize_grids(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x20), M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x24));
        R32A(DH_SCENE_INDEX_ADDR) = DH_SCENE_PLAY_ENTRY;
        TASK_U8(task, state) = 1;
        /* fall through */
    case 1:
        g = R32A(DH_SCENE_INDEX_ADDR);
        if (panel_dir((void *)CTRL1, 0x80) || panel_dir((void *)CTRL2, 0x80)) {
            g = (g >= 81) ? 0 : g + 1;
            R32A(DH_SCENE_INDEX_ADDR) = g;
        }
        g = R32A(DH_SCENE_INDEX_ADDR);
        if (panel_dir((void *)CTRL1, 0x40) || panel_dir((void *)CTRL2, 0x40)) {
            g = (g <= 0) ? 81 : g - 1;
            R32A(DH_SCENE_INDEX_ADDR) = g;
        }
        R32A(DH_UI_CURSOR_COLUMN_ADDR) = 0xD;
        R32A(DH_UI_CURSOR_ROW_ADDR) = 5;
        ((void (*)(const void *, s32, s32))PA(0x060902A0))(
            (const void *)0x06050DE4,
            R32A(0x0609031C + R32A(DH_SCENE_INDEX_ADDR) * 4),
            R32A(DH_SCENE_INDEX_ADDR));
        if ((RU16A(CTRL1 + 0x4) & 0x7) || (RU16A(CTRL2 + 0x4) & 0x7))
            TASK_U8(task, state) = 2;
        return;
    case 2:
        dir = MP(TASK_PTR(task, parent), 0x8);
        M32(dir, 0x3C) |= 1;
        *(volatile s16 *)(uintptr_t)VDP2_TVMD_ADDR = (s16)(MU16(DISP, 0x0) & 0x7FFF);
        R32A(0x06099578) = 0;
        kill_objects_above_id(0);
        return;
    default:
        return;
    }
}

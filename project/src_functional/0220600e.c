/* scene_credit_join_task_entry [0x0220600e]: Runs the credit/join display states and resident coin/start gates before advancing the scene. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220600e (scene_credit_join_sm) - the CREDIT / player-join UI state machine.
 *
 * This is the attract-mode "insert coin / press start to join" scene handler
 * (one scene off-slot handler; ABI: handler(task)). task = r4(arg0);
 * entity = task->unk8 (r12). Per-frame state machine dispatched on task->unk10
 * (byte @+0x10), a cmp/eq chain rendered faithfully as a C switch (states 0..3;
 * NO jump table for the main dispatch). Common tail every frame: call the UI
 * per-frame service (*0x06090298)(), set 0x0609869C=1.
 *
 * It renders text through the two HUD print thunks (RAM fn-ptr slots):
 *   (*0x060902A0)(fmt, ...)  - formatted text thunk ("ui_printf")
 *   (*0x0609029C)(desc)      - glyph/descriptor thunk ("ui_glyph")
 * both take their screen position from the HUD cursor globals set immediately
 * before each call: 0x060997A4 = color, 0x060997A8 = mode (confirmed in
 * notes/symbols.txt). ROM string/format table lives at 0x06050DF8..0x06050E3C;
 * ROM player-number glyph table at 0x060506F0 (indexed by panel count byte).
 *
 * THE 0x06090304 DISPATCH (faithfully decoded, NOT stubbed): 0x06090304 is a
 * RAM-resident descriptor table (the per-player insert-coin / join "digit"
 * descriptors - see attract_handler @0x02204868, which maintains the same table).
 * It is INDEXED BY PLAYER-READY STATE: for a panel slot whose count byte
 * (panelbuf->unk0) is <=1 the code selects entry [4], otherwise entry [5], and
 * passes the selected descriptor to the glyph thunk (*0x0609029C). Entries 4/5
 * are the "not-ready" / "ready" join glyphs. The entries themselves are RAM
 * pointers populated at runtime (not ROM constants), so they are rendered here as
 * the real indexed indirect load+call - the index computation IS the dispatch.
 *
 * BIOS/service fn-ptr slots used:
 *   (*0x06000660)(buf 0x060987D8) - read the ST-V control panel into the buffer
 *   (*0x06000664)(player)         - consume one coin for `player`; nonzero = ok
 *   (*0x06000680)(arg)            - coin/sound service
 * 0x06000654 = BIOS panel status word (bit 0x200 = 2-player panel present; low 7
 * bits = the insert-coin countdown timer, 0x7F = idle).
 *
 * Panel buffer 0x060987D8 (P1) / 0x060987DC (P2): unk0 = count byte, unk1/unk2 =
 * signed glyph offset adjustments, unk10 = status flags (bit2 0x04 = 2-player
 * panel full, bit0 0x01 = slot already taken). Join-state bytes: 0x06098674 (P1),
 * 0x06098675 (P2) (==1 means already joined). Coin-accept latches: 0x0609880C
 * (P1), 0x06098810 (P2). Panel action bytes: 0x060987DB (P1), 0x060987DF (P2)
 * (==2 is START). 0x060987E8 bit2 selects the 2-player panel path.
 *
 * STATE 3 (commit) installs attract_handler (0x02204868) as the next task handler
 * (*(*0x06098678) = 0x02204868), sets the scene-director advance bit
 * (task->unk14->unk8->unk3C |= 1), blanks the VDP2 display, clears the start/EDSR
 * gates, and tears down live objects via kill_objects_above_id(0) - i.e. it hands
 * the credit scene off to the attract/coin-wait pivot. When state 3 is entered
 * before the exit object has finished (entity->unk3C bit1 set, or unk60==0) the
 * original code BRANCHES BACK INTO the state-2 render body for that frame; this is
 * reproduced here by calling the shared credit_render() helper.
 *
 * Branches resolved via braf_resolve (bsrf PIC calls) + ptr_resolve (mova+add data
 * ptrs: spawn templates 0x022100f8 / 0x022101ce, aux-obj data 0x0220686a, attract
 * handler 0x02204868). fidelity: HIGH - hand-traced vs braf_rewrite --asm over the
 * full extent 0x20600e..0x20686a. */

#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))                 /* fn-ptr/ptr slot */
#define TABP(a)  (*(void **)(uintptr_t)(a))                 /* RAM table entry */

#define G_COLOR  DH_UI_CURSOR_COLUMN_ADDR   /* HUD cursor color  */
#define G_MODE   DH_UI_CURSOR_ROW_ADDR   /* HUD cursor mode   */
#define UI_PRINTF  ((void (*)(const void *, ...))PA(0x060902A0))
#define UI_GLYPH   ((void (*)(const void *))PA(0x0609029C))

#define PANEL1   0x060987D8
#define PANEL2   0x060987DC

extern void  video_state_init(s32 mode);                /* 0x02216154 */
extern void  vdp2_setup_largefont(s32 arg);             /* 0x02222e44 */
extern void  vdp1_draw_kick(void);                      /* 0x02214d26 */
extern void  upload_texture_groups(s32 group);          /* 0x022150d4 */
extern void  vdp2_upload_pattern_groups(s32 arg);                    /* 0x02215228 */
extern void  model_load_scene_records(s32 arg);                    /* 0x02223ac0 */
extern void  entity_system_reset(void);                 /* 0x02227a84 */
extern void  effect_pool_reset_scene(void);                       /* 0x0223c95c */
extern void  stage_vram_loader(void *task, void *sd);   /* 0x02207224 */
extern void  scene_layer_loader_spawn(void *task, void *sd); /* 0x02207772 */
extern void  scene_display_initialize_entry(void *task, void *sd);       /* 0x022082be */
extern void *spawn_obj_0x6c(void *task, void *tmpl);    /* 0x0220fece */
extern void  kill_objects_above_id(u8 id);              /* 0x0220fdc8 */

/* select the join-digit descriptor from the 0x06090304 RAM table by player-ready
 * state: entry [4] if the slot count byte is <=1, else entry [5]. */
static void *join_digit_desc(void *panelbuf) {
    s32 idx = ((s32)MU8(panelbuf, 0) <= 1) ? 4 : 5;
    return TABP(0x06090304 + idx * 4);
}

/* render one panel slot's glyph group: the player-ready digit, the player-number
 * glyph (ROM table 0x060506F0), and the name glyph (scratch-built from the ROM
 * 3-byte record `rec`, offset by the panel's unk1/unk2, or the default 0x06050D50).
 * The name glyph deliberately sets NO cursor: it reuses the player-number glyph's
 * cursor (col_num / mode 0xB), which the thunk auto-advances. */
static void render_slot(void *panelbuf, s32 col_digit, s32 col_num, const void *rec) {
    u8 scratch[3];

    R32A(G_COLOR) = col_digit; R32A(G_MODE) = 0xB;
    UI_GLYPH(join_digit_desc(panelbuf));

    R32A(G_COLOR) = col_num;   R32A(G_MODE) = 0xB;
    UI_GLYPH((const void *)(uintptr_t)(0x060506F0 + MU8(panelbuf, 0) * 4));

    if (M8(panelbuf, 1) != 0) {
        scratch[0] = MU8(rec, 0) + MU8(panelbuf, 1);
        scratch[1] = MU8(rec, 1) + MU8(panelbuf, 2);
        scratch[2] = MU8(rec, 2);
        UI_GLYPH(scratch);
    } else {
        UI_GLYPH((const void *)0x06050D50);
    }
}

/* STATE 2 body: poll the panel, render the join UI, handle coin/start input.
 * Shared between case 2 and the state-3 "not finished" fall-back. */
static void credit_render(void *task, void *entity) {
    void *ctrl = (void *)0x06098794;   /* g_ctrl_struct; [+4] = cooked ctrl word */
    void *spawned;
    s32   timer;

    ((void (*)(void *))PA(0x06000660))((void *)PANEL1);   /* read panel */

    /* header prompt line */
    if (RU16A(0x06000654) & 0x200) {      /* 2-player panel */
        R32A(G_COLOR) = 0xE; R32A(G_MODE) = 8;
        UI_PRINTF((const void *)0x06050DF8);
        R32A(G_COLOR) = 0xD; R32A(G_MODE) = 9;
        UI_PRINTF((const void *)0x06050E0C);
    } else {                              /* single-player panel */
        R32A(G_COLOR) = 0xE; R32A(G_MODE) = 8;
        UI_PRINTF((const void *)0x06050DF8);
        R32A(G_COLOR) = 0xF; R32A(G_MODE) = 9;
        UI_PRINTF((const void *)0x06050E20);
    }

    if (!(MU8((void *)PANEL1, 0x10) & 4)) {           /* single credit line */
        s32 cval = (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 1)
                   ? R32A(0x06090300)
                   : R32A(0x060902F4 + RU8A(0x060987DB) * 4);
        R32A(G_COLOR) = 0xC; R32A(G_MODE) = 0xA;
        UI_PRINTF((const void *)0x06050E30, cval);

        if (!(MU8((void *)PANEL1, 0x10) & 1))
            render_slot((void *)PANEL1, 0x10, 0x18, (const void *)0x06050E39);
    } else {                                          /* two credit lines */
        s32 c1 = (RU8A(DH_PLAYER_JOIN_STATES_ADDR) == 1)
                 ? R32A(0x06090300)
                 : R32A(0x060902F4 + RU8A(0x060987DB) * 4);
        s32 c2;
        R32A(G_COLOR) = 1; R32A(G_MODE) = 0xA;
        UI_PRINTF((const void *)0x06050E30, c1);

        c2 = (RU8A(0x06098675) == 1)
             ? R32A(0x06090300)
             : R32A(0x060902F4 + RU8A(0x060987DF) * 4);
        R32A(G_COLOR) = 0x17; R32A(G_MODE) = 0xA;
        UI_PRINTF((const void *)0x06050E30, c2);

        if (!(MU8((void *)PANEL1, 0x10) & 1)) {
            render_slot((void *)PANEL1, 0xA, 0x12, (const void *)0x06050E33);
            render_slot((void *)PANEL2, 0x16, 0x1E, (const void *)0x06050E36);
        }
    }

    /* insert-coin countdown digit */
    timer = RU16A(0x06000654) & 0x7F;
    if (timer != 0x7F) {
        R32A(G_COLOR) = 0x12; R32A(G_MODE) = 0xD;
        UI_PRINTF((const void *)0x06050E3C, timer);
    }

    if (timer == 0 && M32(entity, 0x40) == 0) {
        /* countdown expired: consume coins for any un-joined player */
        if (RU8A(0x060987E8) & 4) {       /* 2-player panel */
            if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) != 1 &&
                ((int (*)(int))PA(0x06000664))(0) != 0)
                RU16A(0x0609880C) = 1;
            else if (RU8A(0x06098675) != 1 &&
                     ((int (*)(int))PA(0x06000664))(1) != 0)
                RU16A(0x06098810) = 1;
        } else if (RU8A(DH_PLAYER_JOIN_STATES_ADDR) != 1 && RU8A(0x06098675) != 1 &&
                   ((int (*)(int))PA(0x06000664))(0) != 0) {
            RU16A(0x0609880C) = 1;
        }
        M32(entity, 0x40) = 1;
        TASK_U8(task, state) = 3;
        if (M32(entity, 0x60) == 0) {
            M32(entity, 0x60) = 1;
            spawned = spawn_obj_0x6c(task, (void *)0x022101CE);
            M32(MP(spawned, 0x8), 0x50) = -0x14;
            ((void (*)(s32))PA(0x06000680))(0);
        }
    } else {
        /* countdown running: handle START-button joins */
        u16 cw = RU16A((uintptr_t)ctrl + 4);          /* cooked ctrl word */
        if (RU8A(0x060987E8) & 4) {                   /* 2-player panel */
            if (((cw & 0x10) && RU8A(0x060987DB) == 2) ||
                ((cw & 0x20) && RU8A(0x060987DF) == 2)) {
                if ((cw & 0x10) && RU16A(0x0609880C) == 0 &&
                    ((int (*)(int))PA(0x06000664))(0) != 0)
                    RU16A(0x0609880C) = 1;
                if ((RU16A((uintptr_t)ctrl + 4) & 0x20) && RU16A(0x06098810) == 0 &&
                    ((int (*)(int))PA(0x06000664))(1) != 0)
                    RU16A(0x06098810) = 1;
                if (RU16A(0x0609880C) == 1 || RU16A(0x06098810) == 1) {
                    if (M32(entity, 0x60) == 0) {
                        M32(entity, 0x60) = 1;
                        spawned = spawn_obj_0x6c(task, (void *)0x022101CE);
                        M32(MP(spawned, 0x8), 0x50) = -0x14;
                        ((void (*)(s32))PA(0x06000680))(0);
                    }
                    M32(entity, 0x40) = 1;
                    TASK_U8(task, state) = 3;
                }
            }
        } else {                                      /* single-player panel */
            if ((cw & 0x10) && RU8A(0x060987DB) == 2 && RU16A(0x0609880C) == 0 &&
                ((int (*)(int))PA(0x06000664))(0) != 0)
                RU16A(0x0609880C) = 1;
            if ((RU16A((uintptr_t)ctrl + 4) & 0x20) && RU8A(0x060987DB) == 2 &&
                RU16A(0x06098810) == 0 &&
                ((int (*)(int))PA(0x06000664))(1) != 0)
                RU16A(0x06098810) = 1;
            if (RU16A(0x0609880C) == 1 || RU16A(0x06098810) == 1) {
                if (M32(entity, 0x60) == 0) {
                    M32(entity, 0x60) = 1;
                    spawned = spawn_obj_0x6c(task, (void *)0x022101CE);
                    M32(MP(spawned, 0x8), 0x50) = -0x14;
                    ((void (*)(s32))PA(0x06000680))(0);
                }
                M32(entity, 0x40) = 1;
                TASK_U8(task, state) = 3;
            }
        }
    }
}

void scene_credit_join_sm(void *task) {
    void *entity = TASK_PTR(task, work);
    void *sd;
    void *disp;
    void *aux, *aux_e;
    void *spawned;

    switch (TASK_U8(task, state)) {
    case 0:   /* ---- INIT: build the credit-scene display + spawn intro obj ---- */
        R32A(0x06099578) = 0;                 /* EDSR/start gate off */
        video_state_init(-1);

        disp = PA(DH_VDP2_REGISTER_SHADOW_ADDR);
        M16(disp, 0xE)  = (M16(disp, 0xE) & ~0x300) | 0x300;
        M16(disp, 0xE)  = (M16(disp, 0xE) & ~0xFF)  | 0x23;
        M32(disp, 0x10) = -1;
        M32(disp, 0x14) = 0x454FFFFF;         /* (disp+0x10)->unk4 */
        M32(disp, 0x18) = -1;                 /* (disp+0x10)->unk8 */
        M32(disp, 0x1C) = 0x0137FFFF;         /* (disp+0x10)->unkC */
        vdp2_setup_largefont(7);
        ((void (*)(s32))PA(0x06090294))(7);   /* UI/display init thunk */
        R32A(0x060986B8) = 0;
        vdp1_draw_kick();

        disp = PA(DH_VDP2_REGISTER_SHADOW_ADDR);
        M16(disp, 0x0)  = (M16(disp, 0x0) & ~0x30) | 0x10;
        M16(disp, 0x0)  = (M16(disp, 0x0) & ~0x7)  | 0x3;
        M16(disp, 0xE0) = (M16(disp, 0xE0) & ~0xF) | 0xF;
        M16(disp, 0xF8) = 0x604;
        M16(disp, 0xFA) = 0x700;
        M16(disp, 0xFC) = 3;
        M16(disp, 0xF0) = 5;
        M16(disp, 0xF2) = 0;
        M16(disp, 0xF4) = 0;
        M16(disp, 0xF6) = 0;
        M16(disp, 0x0)  = (M16(disp, 0x0) & 0x7FFF) | 0x8000;

        sd = PA(DH_SCENE_DESCRIPTOR_ADDR);                  /* scene descriptor */
        upload_texture_groups(SCENE_M32(sd, texture_groups));
        vdp2_upload_pattern_groups(SCENE_M32(sd, pattern_groups));
        model_load_scene_records(SCENE_M32(sd, model_list));

        aux = PA(DH_VIEW_TASK_ADDR);                 /* aux display object */
        MP(aux, 0x0)  = (void *)0x0220686A;   /* aux->unk0 = ROM data @0x0220686a */
        MU8(aux, 0x10) = 0;
        aux_e = MP(aux, 0x8);
        M32(aux_e, 0x8)  = 0;
        M32(aux_e, 0xC)  = 0;
        M32(aux_e, 0x10) = 0;
        M16(aux_e, 0x24) = 0;
        M16(aux_e, 0x26) = 0;
        M16(aux_e, 0x28) = 0;
        entity_system_reset();
        effect_pool_reset_scene();

        sd = PA(DH_SCENE_DESCRIPTOR_ADDR);
        stage_vram_loader(task, sd);
        scene_layer_loader_spawn(task, sd);
        scene_display_initialize_entry(task, sd);

        M32(entity, 0x60) = 0;
        M32(entity, 0x40) = 0;
        R32A(DH_PLAY_SESSION_MODE_ADDR) = 1;                 /* credit-scene armed */
        R32A(0x060987D0) = 1;
        RU8A(0x0609861C) = 1;
        R32A(0x06099578) = 1;                 /* EDSR/start gate on */

        spawned = spawn_obj_0x6c(task, (void *)0x022100F8);
        M32(MP(spawned, 0x8), 0x50) = 0x40;
        R32A(DH_SCENE_EVENT_FLAGS_ADDR) |= 0x20000;
        TASK_U8(task, state) = 1;
        break;

    case 1:   /* ---- wait for the intro object to finish, then arm coin svc ---- */
        if (!(M32(entity, 0x3C) & 2)) {
            ((void (*)(s32))PA(0x06000680))(1);
            TASK_U8(task, state) = 2;
        }
        break;

    case 2:   /* ---- MAIN: poll panel, render join UI, handle coin/start ---- */
        credit_render(task, entity);
        break;

    case 3:   /* ---- COMMIT: hand off to attract_handler (0x02204868) ---- */
        if (!(M32(entity, 0x3C) & 2) && M32(entity, 0x60) != 0) {
            void *director_e;
            MP(PA(0x06098678), 0x0) = (void *)0x02204868; /* install next handler */
            director_e = MP(TASK_PTR(task, parent), 0x8);         /* task->unk14->unk8 */
            M32(director_e, 0x3C) |= 1;                    /* director advance bit */
            *(volatile s16 *)(uintptr_t)VDP2_TVMD_ADDR =
                (s16)(M16(PA(DH_VDP2_REGISTER_SHADOW_ADDR), 0x0) & 0x7FFF);  /* blank VDP2 display */
            R32A(0x06099578) = 0;
            RU8A(0x0609861C) = 0;
            kill_objects_above_id(0);
            R32A(0x060987D0) = 0;
        } else {
            credit_render(task, entity);       /* not finished: re-run the UI */
        }
        break;
    }

    /* ---- common per-frame tail ---- */
    ((void (*)(void))PA(0x06090298))();
    R32A(0x0609869C) = 1;
}

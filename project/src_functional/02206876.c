/* scene_game_control_task_entry [0x02206876]: Sequences scene-81 resources, start gates, fades and parent scene completion. */
#include "dh_work.h"
#include "dh.h"
/* 0x02206876 (scene81_game_sm) - the GAME scene (scene 81) per-frame state machine.
 *
 * This is the handler installed as scene-81's "off14" setup (descriptor 0x06073fb0,
 * off14 = rt 0x0600a876). ABI (task): task = r4(arg0); entity = task->unk8 (r11).
 *
 * PART 1 - the START gate (cart 0x02206876..0x2068f0). r14 = control struct 0x06098794;
 *   the cooked control word is [r14+4] = 0x06098798 (bit4 0x10 = P1 start-enable,
 *   bit5 0x20 = P2 start-enable; these are BIOS/system-provided, never set by cart code).
 *   If neither bit is set the panel is NOT polled and the gate is skipped. If set, it
 *   calls the ST-V BIOS control-panel service (*0x06000660)(buf 0x060987D8) and, on a
 *   panel START (action byte 0x060987DB==2 for P1 / 0x060987DF==2 for P2, 0x060987E8 bit2
 *   selects the 2-player path), forces task->state(+0x10)=5 and clears entity->unk3c.
 *
 * PART 2 - dispatch on task->state (byte @+0x10), a cmp/eq chain rendered faithfully as a
 *   switch (NO jump table in this function). Progression is 0 -> 4 -> 1 -> 2 -> 3 -> 5:
 *     state 0: init the game display shadow (*0x06098F08): VDP2 regs, large-font setup,
 *              vdp1_draw_kick, upload stage tile groups (scenedesc *0x0609866C fields),
 *              entity-system reset; -> state 4.
 *     state 4: optional SCSP upload (scenedesc->unkC), run scene81_stage_setup(task,scenedesc)
 *              [loads the stage graphics to VDP2 VRAM], arm a 0x6C-frame timer @entity+0x50,
 *              clear character selector 0x0609856C / 0x06098580; -> state 1.
 *     state 1: count the timer down; on expiry spawn an intro object (spawn_obj_0x6c,
 *              template 0x022101ce), seed its unk8->unk50=8; -> state 2.
 *     state 2: when entity->unk3c bit1 (busy) is clear, spawn object (template 0x022100f8),
 *              seed unk8->unk50=-16, entity->unk60=0x20000; -> state 3.
 *     state 3: decay entity->unk60; when bit1 clear spawn object (template 0x022101ce),
 *              seed unk8->unk50=-2; -> state 5.
 *     state 5 (cart 0x02206c40, THE GAME-START COMMIT): when entity->unk3c bit1 is clear,
 *              blank the VDP2 display (0x25F80000 = shadow->unk0 & 0x7FFF), clear the
 *              EDSR/start gates (0x06099578=0, 0x0609861C=0), SET the scene-director advance
 *              bit (task->unk14->unk8->unk3c |= 1) so the director leaves the game scene,
 *              and tear down live objects via kill_objects_above_id(0) (0x0220fdc8).
 *              NOTE: this state COMMITS the game start (teardown + director hand-off), it
 *              does not itself allocate the player actor; the player spawn is installed
 *              downstream of the director advance. Faithfully decoded, not stubbed.
 *   Common tail (0x02206c6a): call the per-frame service (*0x06090298)(), set 0x0609869C=1.
 *
 * Branches resolved via braf_resolve (bsrf PIC calls) + ptr_resolve (mova+add data ptrs).
 * fidelity: HIGH - hand-traced vs m2c_fn --asm (full extent 0x206876..0x206c88). */

#define RU8A(a)  (*(u8  *)(uintptr_t)(a))
#define RU16A(a) (*(u16 *)(uintptr_t)(a))
#define R32A(a)  (*(s32 *)(uintptr_t)(a))
#define PA(a)    (*(void **)(uintptr_t)(a))
#define DISP     (*(void **)(uintptr_t)DH_VDP2_REGISTER_SHADOW_ADDR)   /* game display-shadow struct ptr */

extern void  video_state_init(s32 mode);               /* 0x02216154 */
extern void  vdp2_setup_largefont(s32 arg);            /* 0x02222e44 */
extern void  vdp1_draw_kick(void);                     /* 0x02214d26 */
extern void  vdp1_upload_texture_groups(s32 group);                 /* 0x022150d4 upload_texture_groups */
extern void  vdp2_upload_pattern_groups(s32 arg);                   /* 0x02215228 */
extern void  model_load_scene_records(s32 arg);                   /* 0x02223ac0 */
extern void  entity_system_reset(void);                /* 0x02227a84 */
extern void  effect_pool_reset_scene(void);                      /* 0x0223c95c */
extern void  vdp2_apply_sprite_commands(void *arg);                 /* 0x02216b14 */
extern void  stage_initialize_grids(s32 a, s32 b);              /* 0x02216c38 */
extern void *spawn_obj_0x6c(void *task, void *tmpl);   /* 0x0220fece */
extern void  sound_send_command(void *data, s32 len);  /* 0x02215e20 */
extern void  sound_cue_dispatch(void);                 /* 0x02215fac */
extern void  sound_queue_push(s32 req);                /* 0x02215f6c */
extern void  kill_objects_above_id(u8 id);             /* 0x0220fdc8 */
extern void  scene81_stage_setup(void *task, void *scenedesc); /* 0x02206e2e */

void scene81_game_sm(void *task) {
    void *entity = TASK_PTR(task, work);
    void *sd;
    void *o;

    /* ---- PART 1: START gate ---- */
    if (RU16A(0x06098798) & 0x30) {
        ((void (*)(void *))PA(0x06000660))((void *)0x060987D8);  /* panel poll */
        if (RU8A(0x060987E8) & 4) {                              /* 2-player panel */
            u16 cw = RU16A(0x06098798);
            if (((cw & 0x10) && RU8A(0x060987DB) == 2) ||
                ((cw & 0x20) && RU8A(0x060987DF) == 2)) {
                M32(entity, 0x3C) = 0;
                TASK_U8(task, state) = 5;
            }
        } else if ((RU16A(0x06098798) & 0x30) && RU8A(0x060987DB) == 2) {
            M32(entity, 0x3C) = 0;
            TASK_U8(task, state) = 5;
        }
    }

    /* ---- PART 2: state dispatch ---- */
    switch (TASK_U8(task, state)) {
    case 0:
        R32A(0x06099578) = 0;
        video_state_init(-1);
        MU16(DISP, 0xE) = (u16)((MU16(DISP, 0xE) & 0xFCFF) | 0x0300);
        MU16(DISP, 0xE) = (u16)((MU16(DISP, 0xE) & 0xFF00) | 0x23);
        M32(DISP, 0x10) = -1;
        M32(DISP, 0x14) = 0x454FFFFF;
        M32(DISP, 0x18) = -1;
        M32(DISP, 0x1C) = 0x0137FFFF;
        vdp2_setup_largefont(8);
        ((void (*)(void))PA(0x06090294))();          /* HWRAM display hook */
        MU16(DISP, 0xF8) = 0x0603;
        MU16(DISP, 0xFA) = 0x0700;
        MU16(DISP, 0xFC) = 4;
        MU16(DISP, 0xF0) = 5;
        MU16(DISP, 0xF2) = 0;
        MU16(DISP, 0xF4) = 0;
        MU16(DISP, 0xF6) = 0;
        MU16(DISP, 0x114) = 0;
        MU16(DISP, 0x116) = 0;
        MU16(DISP, 0x118) = 0;
        vdp1_draw_kick();
        MU16(DISP, 0x0) = (u16)((MU16(DISP, 0x0) & ~0x30) | 0x10);
        MU16(DISP, 0x0) = (u16)((MU16(DISP, 0x0) & ~0x7) | 0x3);
        vdp2_apply_sprite_commands((void *)0x060506DC);
        MU16(DISP, 0xE0) = (u16)((MU16(DISP, 0xE0) & ~0xF) | 0xF);
        MU16(DISP, 0x0) = (u16)((MU16(DISP, 0x0) & 0x7FFF) | 0x8000);
        sd = PA(DH_SCENE_DESCRIPTOR_ADDR);
        {
            void *t = PA(DH_VIEW_TASK_ADDR);
            M32(t, 0x0) = SCENE_M32(sd, camera_definition);
            MU8(t, 0x10) = 0;
        }
        vdp1_upload_texture_groups(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x0));
        vdp2_upload_pattern_groups(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x4));
        model_load_scene_records(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x8));
        entity_system_reset();
        effect_pool_reset_scene();
        stage_initialize_grids(M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x20), M32(PA(DH_SCENE_DESCRIPTOR_ADDR), 0x24));
        TASK_U8(task, state) = 4;
        break;
    case 4:
        sd = PA(DH_SCENE_DESCRIPTOR_ADDR);
        if (MP(sd, 0xC) != (void *)0)
            sound_send_command(MP(sd, 0xC), 0x6C000);
        scene81_stage_setup(task, PA(DH_SCENE_DESCRIPTOR_ADDR));
        M32(PA(0x060B8B90), 0x10) = 0x8000;
        RU8A(0x0609861C) = 1;
        R32A(0x06099578) = 1;
        M32(entity, 0x50) = 0x6C;
        sound_cue_dispatch();
        R32A(DH_SCENE_EVENT_FLAGS_ADDR) = 0;
        R32A(0x0609856C) = 0;
        TASK_U8(task, state) = 1;
        break;
    case 1:
        {
            s32 t = M32(entity, 0x50) - 1;
            M32(entity, 0x50) = t;
            if (t > 0)
                break;
        }
        MU16(DISP, 0x114) = 0;
        MU16(DISP, 0x116) = 0;
        MU16(DISP, 0x118) = 0;
        o = spawn_obj_0x6c(task, (void *)0x022101CE);
        M32(MP(o, 0x8), 0x50) = 8;
        if (R32A(DH_SOUND_EFFECTS_ENABLED_ADDR) != 0)
            sound_queue_push(0xA01403FF);
        TASK_U8(task, state) = 2;
        break;
    case 2:
        if (!(M32(entity, 0x3C) & 2)) {
            MU16(DISP, 0x114) = 0xFF;
            MU16(DISP, 0x116) = 0xFF;
            MU16(DISP, 0x118) = 0xFF;
            o = spawn_obj_0x6c(task, (void *)0x022100F8);
            M32(MP(o, 0x8), 0x50) = -16;
            TASK_U8(task, state) = 3;
            M32(entity, 0x60) = 0x20000;
        }
        break;
    case 3:
        M32(entity, 0x60) -= 0xF0F;
        if (!(M32(entity, 0x3C) & 2)) {
            MU16(DISP, 0x114) = 0;
            MU16(DISP, 0x116) = 0;
            MU16(DISP, 0x118) = 0;
            o = spawn_obj_0x6c(task, (void *)0x022101CE);
            M32(MP(o, 0x8), 0x50) = -2;
            TASK_U8(task, state) = 5;
        }
        break;
    case 5:
        if (!(M32(entity, 0x3C) & 2)) {
            void *dir;
            *(u16 *)(uintptr_t)VDP2_TVMD_ADDR = (u16)(MU16(DISP, 0x0) & 0x7FFF);
            R32A(0x06099578) = 0;
            RU8A(0x0609861C) = 0;
            dir = MP(TASK_PTR(task, parent), 0x8);
            M32(dir, 0x3C) |= 1;
            kill_objects_above_id(0);
        }
        break;
    default:
        break;
    }

    /* ---- common tail ---- */
    ((void (*)(void))PA(0x06090298))();
    R32A(0x0609869C) = 1;
}

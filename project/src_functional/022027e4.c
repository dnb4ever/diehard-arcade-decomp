/* scene_bootstrap_task [0x022027e4]: Sequences graphics reload, stage reset, fade tasks and parent completion. */
#include "dh_work.h"
#include "dh.h"
/* 0x022027e4 - scene bootstrap STATE MACHINE (task handler, 4 states). task->unk8
 * = entity; task->unk10 = state. Brings a scene online then tears it down:
 *   state 0: func_02217d6c(task); state -> 1.
 *   state 1: func_02217f04(task); install handler 0x0220f6f6 into the scene obj
 *     g_0609882c; set VDP2 shadow (g_06098f08) unk114/116/118 = -255; spawn the
 *     scene's HUD/gravity/layer tasks (spawn_obj_0x6c & func_0220feec with
 *     handlers 0x2100f8, thrown_object_gravity_task 0x202ba8, scene_hud_task_init
 *     0x20340c, 0x21fe00); stash the last spawn in entity->unk64 and seed its
 *     entity fields; state -> 2.
 *   state 2: when g_06098664 == 1, spawn handler 0x2101ce, sound cue 0xa00338ff,
 *     state -> 3.
 *   state 3: func_02216d58(); once entity->unk3c bit1 is set, enable VDP2 display
 *     (TVMD = S->unk0 & 0x7fff), clear the scene-ready globals, sound 0xa00003ff,
 *     and TAIL-CALL func_0220fdc8((s8)task->unkE + 0xff).
 *
 * fidelity: HIGH except MED on two calls with inherited r4 (func_02215fac gets the
 * just-spawned obj; func_02216d58 gets task) - flagged inline. r12 jsr target
 * resolved to spawn_obj_0x6c (0x20fece); all bsrf via braf_resolve.py; all
 * mova+add handler pointers resolved by hand. The final braf is a tail call. */

#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))
#define REG_TVMD (*(volatile u16 *)(uintptr_t)VDP2_TVMD_ADDR)

extern void  scene_reload_graphics(void *task);                     /* 0x217d6c */
extern void  scene_reset_stage(void *task);                     /* 0x217f04 */
extern void  audio_queue_scene_sound(void *obj);                      /* 0x215fac (r4 inherited) */
extern void  stage_draw_visible_cells(void *task);                     /* 0x216d58 (r4 inherited) */
extern void  sound_queue_push(s32 cmd);                       /* 0x215f6c sound_queue_push */
extern void  task_cancel_below_depth(s32 a);                         /* 0x20fdc8 (tail) */
extern void *spawn_obj_0x6c(void *parent, void *handler);  /* 0x20fece */
extern void *task_spawn_child_work88(void *parent, void *handler);   /* 0x20feec */

/* scene handlers (referenced by address) */
extern void camera_step_scene_definition(void *task);
extern void display_ramp_to_zero(void *task);
extern void scene_prop_gravity_task(void *task);   /* thrown_object_gravity_task */
extern void hud_install_status_task(void *task);   /* scene_hud_task_init */
extern void model_draw_segment_list_task(void *task);
extern void display_ramp_to_limit(void *task);

void scene_bootstrap_task(void *task) {
    void *entity = TASK_PTR(task, work);
    void *S = GP(DH_VDP2_REGISTER_SHADOW_ADDR);
    void *obj;

    switch (TASK_U8(task, state)) {
    case 0:
        scene_reload_graphics(task);
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);   /* -> 1 */
        break;

    case 1: {
        void *q;
        scene_reset_stage(task);
        q = GP(DH_VIEW_TASK_ADDR);
        MP(q, 0) = (void *)camera_step_scene_definition;
        M8(q, 0x10) = 0;
        MU16(S, 0x114) = (u16)-255;
        MU16(S, 0x116) = (u16)-255;
        MU16(S, 0x118) = (u16)-255;
        obj = spawn_obj_0x6c(task, (void *)display_ramp_to_zero);
        M32(MP(obj, 8), 0x50) = 32;
        audio_queue_scene_sound(obj);                           /* r4 inherited (=obj) */
        G32(DH_READY_PLAYER_MASK_ADDR) = 3;
        task_spawn_child_work88(task, (void *)scene_prop_gravity_task);
        spawn_obj_0x6c(task, (void *)hud_install_status_task);
        obj = task_spawn_child_work88(task, (void *)model_draw_segment_list_task);
        M32(entity, 0x64) = (s32)(uintptr_t)obj;
        {
            void *e3 = MP(obj, 8);
            M32(e3, 8) = 0x00200000;
            M32(e3, 0xc) = 0;
            M32(e3, 0x10) = 0x00200000;
        }
        TASK_S8(task, state) = 2;
        break;
    }

    case 2:
        if (G32(0x06098664) == 1) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
            M32(MP(obj, 8), 0x50) = -16;
            sound_queue_push(0xa00338ff);
            TASK_S8(task, state) = 3;
        }
        break;

    case 3:
        stage_draw_visible_cells(task);                          /* r4 inherited (=task) */
        if (!(M32(entity, 0x3c) & 2)) {
            void *e = MP(TASK_PTR(task, parent), 8);
            M32(e, 0x3c) |= 1;
            REG_TVMD = (u16)(MU16(S, 0) & 0x7fff);
            G32(0x06099578) = 0;
            G32(0x060bacac) &= ~1;
            G32(0x060bacb0) &= ~1;
            G32(0x060bacb4) &= ~1;
            sound_queue_push(DH_SOUND_TRANSITION_DONE_COMMAND);
            G32(0x060986b8) = 0;
            *(s8 *)(uintptr_t)0x0609861c = 0;
            G32(0x06098664) = 0;
            task_cancel_below_depth((s8)TASK_S8(task, depth) + 0xff);  /* tail call */
        }
        break;

    default:
        break;
    }
}

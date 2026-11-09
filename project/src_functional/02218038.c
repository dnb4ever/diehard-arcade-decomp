/* scene_record_bootstrap_task [0x02218038]: Sequences record-driven scene resources, fade channels, readiness and parent notification. */
#include "dh_work.h"
#include "dh.h"
/* 0x02218038  scene_record_bootstrap_sm - the shared record-driven scene bootstrap
 * STATE MACHINE. This is the braf tail-call target of a family of tiny per-scene
 * dispatchers (scene_bootstrap_dispatch_*) which compute
 *     record = <table base> + g_scene->unk29 * 12
 * and `braf 0x218038` with r4 = task, r5 = record.  So the locked ABI is:
 *     r4 = task,  r5 = record  (a 12-byte per-scene-variant descriptor).
 * record layout used here: unk0 = s32 (spawn-param), unk4 = s32 (optional notify
 * arg), unk8 = s16 (obj field seed, state 1), unkA = s16 (obj field seed + sound
 * pan, states 2/3).
 *
 * It is the close sibling of scene_bootstrap_sm (0x022027e4): same handler install,
 * same VDP2 teardown, switch on task->unk10 (task->unk8 = entity):
 *   state 0: func_02217d6c(task); install handler 0x0220f6f6 into the scene obj
 *     g_0609882c; scene-obj->unk10 = 0; task->unk10++.
 *   state 1: func_02217f04(task); *0x06098560 = func_0220feb0(*0x060996d0, record->unk0);
 *     if (record->unk4) func_02233388(task, record->unk4); set VDP2 shadow
 *     (g_06098f08) unk114/116/118 = -255; spawn behaviour 0x022100f8, seed
 *     spawned->unk8->unk50 = record->unk8, func_02215fac(spawned);
 *     func_0221ad58(task); func_0222a8d8(); *0x06098568 = *0x06098564;
 *     entity->unk54 = (*0x0609866c)->unk2B == 1 ? 0x270F : 0x10; task->unk10 = 2;
 *     (falls through into state 2).
 *   state 2: func_0222a8d8(); func_02216d58(task); if g_06098664 == 1 OR the input
 *     gate fires after entity->unk54 counts below 0 and *0x06098670 == 1, spawn
 *     behaviour 0x022101ce, seed spawned->unk8->unk50 = record->unkA, emit a
 *     position-panned sound, task->unk10 = 3.
 *   state 3: func_02216d58(task); once entity->unk3c bit1 is clear, mark
 *     task->unk14->unk8->unk3c bit0, enable VDP2 display (TVMD = S->unk0 & 0x7fff),
 *     clear the scene-ready globals, sound 0xa00003ff, prune objects via
 *     func_0220fdc8((s8)task->unkE + 0xff), *0x06096c90 = 0.
 *
 * fidelity: HIGH except MED on three inherited-r4 calls (func_02215fac gets the
 * just-spawned obj; func_02216d58 gets task) and func_02233388 (2-arg helper named
 * from its call site). r10 jsr target resolved to spawn_obj_0x6c (0x20fece); all
 * bsrf via braf_resolve.py; all mova+add pointers resolved by hand (ptr_resolve.py).
 * Hand-traced statement-for-statement against m2c_fn.py --asm. */

#define G32(a)  (*(s32 *)(uintptr_t)(a))
#define GP(a)   (*(void **)(uintptr_t)(a))
#define REG_TVMD (*(volatile u16 *)(uintptr_t)VDP2_TVMD_ADDR)

extern void  scene_reload_graphics(void *task);                     /* 0x217d6c */
extern void  scene_reset_stage(void *task);                     /* 0x217f04 */
extern void  audio_queue_scene_sound(void *obj);                      /* 0x215fac (r4 inherited) */
extern void  stage_draw_visible_cells(void *task);                     /* 0x216d58 (r4 inherited) */
extern void  sound_queue_push(s32 cmd);                       /* 0x215f6c sound_queue_push */
extern void  audio_queue_scene_gated_id(s16 id);                        /* 0x21603e sound_emit_gated */
extern void  task_cancel_below_depth(s32 a);                         /* 0x20fdc8 kill_objects_above_id */
extern void  scene_spawn_matching_behaviors(s32 obj);                       /* 0x21ad58 spawn_matched_entities */
extern s32   player_refresh_ready_mask(void);                          /* 0x22a8d8 player-ready status */
extern void  scene_spawn_record_list(void *task, s32 arg);           /* 0x233388 (GUESS; record->unk4) */
extern void *task_spawn_child_work6c(void *a, s32 b);                /* 0x20feb0 */
extern void *spawn_obj_0x6c(void *parent, void *handler);  /* 0x20fece */

/* handlers referenced by address */
extern void camera_step_scene_definition(void *task);
extern void display_ramp_to_zero(void *task);
extern void display_ramp_to_limit(void *task);

static s32 iabs(s32 v) { return v < 0 ? -v : v; }

void scene_record_bootstrap_task(void *task, void *record) {
    void *entity = TASK_PTR(task, work);
    void *S = GP(DH_VDP2_REGISTER_SHADOW_ADDR);
    void *obj;

    switch (TASK_U8(task, state)) {
    case 0: {
        void *q = GP(DH_VIEW_TASK_ADDR);
        scene_reload_graphics(task);
        MP(q, 0) = (void *)camera_step_scene_definition;
        M8(q, 0x10) = 0;
        TASK_S8(task, state) = (s8)(TASK_S8(task, state) + 1);   /* -> 1 */
        break;
    }

    case 1:
        scene_reset_stage(task);
        G32(0x06098560) = (s32)(uintptr_t)task_spawn_child_work6c(GP(0x060996d0), M32(record, 0));
        if (M32(record, 4) != 0) {
            scene_spawn_record_list(task, M32(record, 4));
        }
        MU16(S, 0x114) = (u16)-255;
        MU16(S, 0x116) = (u16)-255;
        MU16(S, 0x118) = (u16)-255;
        obj = spawn_obj_0x6c(task, (void *)display_ramp_to_zero);
        M32(MP(obj, 8), 0x50) = M16(record, 8);
        audio_queue_scene_sound(obj);                           /* r4 inherited (=obj) */
        scene_spawn_matching_behaviors((s32)(uintptr_t)task);
        player_refresh_ready_mask();
        G32(0x06098568) = G32(DH_READY_PLAYER_MASK_ADDR);
        M32(entity, 0x54) = (MU8(GP(DH_SCENE_DESCRIPTOR_ADDR), 0x2b) == 1) ? 0x270F : 0x10;
        TASK_S8(task, state) = 2;
        /* fallthrough */

    case 2:
        player_refresh_ready_mask();
        stage_draw_visible_cells(task);                          /* r4 inherited (=task) */
        if (G32(0x06098664) == 1) {
            obj = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
            M32(MP(obj, 8), 0x50) = M16(record, 0xA);
            audio_queue_scene_gated_id((s16)((128 - iabs(M16(record, 0xA))) >> 1));
            TASK_S8(task, state) = 3;
            break;
        }
        {
            s32 old = M32(entity, 0x54);
            M32(entity, 0x54) = old - 1;
            if (old >= 0) {
                break;                               /* still counting down */
            }
        }
        if (!(((MU16(DH_PLAYER_INPUT_RECORDS_ADDR, 0) & 7) && (G32(DH_READY_PLAYER_MASK_ADDR) & 1)) ||
              ((MU16(0x0609874c, 0) & 7) && (G32(DH_READY_PLAYER_MASK_ADDR) & 2)))) {
            break;
        }
        if (G32(DH_PLAY_SESSION_MODE_ADDR) != 1) {
            break;
        }
        obj = spawn_obj_0x6c(task, (void *)display_ramp_to_limit);
        M32(MP(obj, 8), 0x50) = M16(record, 0xA);
        audio_queue_scene_gated_id((s16)((128 - iabs(M16(record, 0xA))) >> 1));
        TASK_S8(task, state) = 3;
        break;

    case 3:
        stage_draw_visible_cells(task);                          /* r4 inherited (=task) */
        if (!(M32(entity, 0x3c) & 2)) {
            void *other = MP(TASK_PTR(task, parent), 8);
            M32(other, 0x3c) |= 1;
            REG_TVMD = (u16)(MU16(S, 0) & 0x7fff);
            G32(0x06099578) = 0;
            G32(0x060bacac) &= ~1;
            G32(0x060bacb0) &= ~1;
            G32(0x060bacb4) &= ~1;
            sound_queue_push(DH_SOUND_TRANSITION_DONE_COMMAND);
            G32(0x060986b8) = 0;
            *(s8 *)(uintptr_t)0x0609861c = 0;
            *(s16 *)(uintptr_t)0x06098812 = 0;
            *(s16 *)(uintptr_t)0x0609880e = 0;
            G32(0x06098664) = 0;
            task_cancel_below_depth((s8)TASK_S8(task, depth) + 0xff);
            G32(0x06096c90) = 0;
        }
        break;

    default:
        break;
    }
}

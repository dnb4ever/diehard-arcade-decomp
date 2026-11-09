/* boot_scene_director_task [0x022016ac]: Sequences boot, attract and scene bring-up while servicing input and scene completion. */
#include "dh_work.h"
#include "dh.h"
/* 0x022016ac - boot/scene-director STATE MACHINE (entry pool-contaminated as
 * 0x022016a0; real prologue at 0x022016ac). This is the per-frame handler that
 * is installed into the director task by boot_display_mode_task (0x02201590)
 * and that sequences the attract/boot/play scene bring-up. task->unk8 = entity;
 * task->unk10 = state byte. g_06098678 holds the current director object whose
 * first slot (*obj) is its draw/update handler fn-pointer.
 *
 * Preamble (runs every frame before dispatch):
 *   - if (*(u16)0x06098798 & 0x0004)      (*0x0600060c)(*0x060987c8);  // BIOS
 *   - if (*(u16)0x06098798 & 0x4000) {
 *         i = *0x06098808;
 *         if (*(u8)(0x060987ec + i + 1) & 1) *0x06098828 = 1;
 *     }
 *
 * Dispatch: faithfully decoded from the braf jump table @ cart 0x201738
 * (10 s16 entries, base = braf_pc+4 = 0x20171a). idx = (u8)state - 1; valid
 * states 1..10, anything else returns. Falls through exactly as the asm does:
 *   1->2, 3->4, 5->6, 7->8, 9->10 (the even states are "wait for ack bit0 of
 *   entity->unk3c, then advance to the next odd state").
 * Jump-table targets (all 10 resolved, none stubbed):
 *   1:0x20174c 2:0x2017a0 3:0x2017cc 4:0x2017ec 5:0x2018a8
 *   6:0x2018d8 7:0x2018e6 8:0x201906 9:0x201914 10:0x201934
 *
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. All bsrf resolved via
 * braf_resolve.py; all mova+add PIC pointers resolved by hand:
 *   noop_handler 0x201b8e; stage handler 0x204868; spawn handlers 0x23673a /
 *   0x236e66 / 0x23358c. r8=-2 (clear bit0 mask), r9=0, r10=4, r13=1 constants
 *   held in callee-saved regs across the whole body. BIOS fn-pointer slots
 *   (0x0600060c/0x06000640/0x06000680/0x06000688) are modeled with the argument
 *   that is live at the call; 0x06000640 returns a free-running timer value
 *   (used as elapsed = now - *0x060987f4 in state 4). */

#define G32(a)   (*(s32 *)(uintptr_t)(a))
#define GU32(a)  (*(u32 *)(uintptr_t)(a))
#define GU16(a)  (*(u16 *)(uintptr_t)(a))
#define GP(a)    (*(void **)(uintptr_t)(a))

/* BIOS / boot-controller function-pointer slots */
#define BIOS_060C (*(void (**)(u32))(uintptr_t)0x0600060c)
#define BIOS_0640 (*(u32  (**)(void))(uintptr_t)0x06000640)
#define BIOS_0680 (*(void (**)(u32))(uintptr_t)0x06000680)
#define BIOS_0688 (*(void (**)(u32))(uintptr_t)0x06000688)

extern void  task_noop(void *task);               /* noop_handler */
extern void  boot_read_region_input(void *ctx);                 /* 0x02201b92 */
extern void  scene_palette_fade_task(void *task);                /* 0x02201c9c */
extern void  scene_bootstrap(void *task);               /* 0x022058a8 */
extern void  scene_advance_task(void *task);               /* 0x02205b20 */
extern u32   smpc_read_intback(void);                     /* 0x0220acf6 (returns handle) */
extern void  resident_add_session_value(u32 elapsed);              /* 0x0220a962 */
extern void  stats_add_counter60(u32 elapsed);              /* 0x0220a992 */
extern void  task_cancel_below_depth(s32 arg);                  /* 0x0220fdc8 */
extern void *spawn_obj_0x6c(void *parent, void *handler); /* 0x0220fece */
extern void  attract_update_task(void *task);               /* stage handler */
extern void  video_menu_bootstrap_sm2(void *task);               /* state-5 spawn handler */
extern void  video_menu_bootstrap_sm3(void *task);               /* state-7 spawn handler */
extern void  video_menu_bootstrap_sm(void *task);               /* state-9 spawn handler */

void boot_scene_director_task(void *task) {
    void *entity = TASK_PTR(task, work);

    /* ---- preamble ---- */
    if (GU16(0x06098798) & 0x0004) {
        BIOS_060C(G32(0x060987c8));
    }
    if (GU16(0x06098798) & 0x4000) {
        u32 i = GU32(0x06098808);
        if (MU8(DH_BIOS_STATUS_BUFFER_ADDR, (s32)i + 1) & 1) {
            G32(0x06098828) = 1;
        }
    }

    /* ---- state dispatch (jump table @ 0x201738) ---- */
    switch (TASK_U8(task, state)) {
    case 1:
        M32(entity, 0x3c) &= ~1;
        G32(DH_PLAY_SESSION_MODE_ADDR) = 0;
        MP(GP(0x06098678), 0) = (void *)task_noop;
        G32(0x060987f4) = (s32)BIOS_0640();
        G32(0x06098800) = (s32)smpc_read_intback();
        M32(entity, 0x3c) |= 1;
        boot_read_region_input(task);
        G32(0x060987c8) = 1;
        TASK_S8(task, state) = 2;
        if (!((GU32(0x06000798) >> 5) & 1)) {
            BIOS_0680(1);
        }
        /* fall through */
    case 2:
        scene_palette_fade_task(task);
        if (M32(entity, 0x3c) & 1) {
            TASK_S8(task, state) = 3;
            scene_bootstrap(task);
            MP(GP(0x06098678), 0) = (void *)attract_update_task;
        }
        return;
    case 3:
        M32(entity, 0x3c) |= 1;
        G32(0x060987f4) = (s32)BIOS_0640();
        G32(0x060987c8) = 0;
        TASK_S8(task, state) = 4;
        BIOS_0688(0);
        /* fall through */
    case 4:
        scene_advance_task(task);
        if (!(M32(entity, 0x3c) & 1)) {
            return;
        }
        MP(GP(0x06098678), 0) = (void *)task_noop;
        resident_add_session_value(BIOS_0640() - (u32)G32(0x060987f4));
        stats_add_counter60(BIOS_0640() - (u32)G32(0x060987f4));
        TASK_S8(task, state) = 1;
        BIOS_0688(1);
        if (!((GU32(0x06000798) >> 5) & 1)) {
            BIOS_060C(G32(0x060987c8));
            TASK_S8(task, state) = 13;
        }
        return;
    case 5:
        G32(DH_PLAY_SESSION_MODE_ADDR) = 1;
        MP(GP(0x06098678), 0) = (void *)task_noop;
        task_cancel_below_depth(0);
        M32(entity, 0x3c) &= ~1;
        spawn_obj_0x6c(task, (void *)video_menu_bootstrap_sm2);
        G32(0x060987c8) = 4;
        TASK_S8(task, state) = 6;
        /* fall through */
    case 6:
        if (M32(entity, 0x3c) & 1) {
            TASK_S8(task, state) = 7;
        }
        return;
    case 7:
        task_cancel_below_depth(0);
        M32(entity, 0x3c) &= ~1;
        spawn_obj_0x6c(task, (void *)video_menu_bootstrap_sm3);
        G32(0x060987c8) = 4;
        TASK_S8(task, state) = 8;
        /* fall through */
    case 8:
        if (M32(entity, 0x3c) & 1) {
            TASK_S8(task, state) = 9;
        }
        return;
    case 9:
        task_cancel_below_depth(0);
        M32(entity, 0x3c) &= ~1;
        spawn_obj_0x6c(task, (void *)video_menu_bootstrap_sm);
        G32(0x060987c8) = 4;
        TASK_S8(task, state) = 10;
        /* fall through */
    case 10:
        if (M32(entity, 0x3c) & 1) {
            GU16(0x0609880c) = 0;
            GU16(0x06098810) = 0;
            TASK_S8(task, state) = 1;
        }
        return;
    default:
        return;
    }
}

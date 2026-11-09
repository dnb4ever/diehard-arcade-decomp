/* hud_install_status_task [0x0220340c]: Selects a per-frame HUD status handler from the BIOS machine configuration. */
#include "dh_work.h"
#include "dh.h"
/* 0x0220340c - scene/HUD task initializer (the "template" spawned by scene_hud_setup_task).
 * entity = task->unk8. First issues the BIOS peripheral/config read (*g_06000660)(0x060987d8),
 * then selects this task's per-frame handler (stored into *task) from the machine-config byte
 * at 0x060987e8:
 *   bit0 set            -> func_02203500
 *   else bit2 set, bit1 set   -> hud_status_service_2p
 *        bit2 set, bit1 clear -> func_02203576
 *        bit2 clear, bit1 set -> func_02203c28
 *        bit2 clear, bit1 clear-> func_02203afc
 * Finally sets entity->unk50 = 0x1F when g_06098668 is 2 or 6, else 0xF.
 * fidelity: HIGH - hand-traced vs m2c_fn --asm. handler addresses are mova base + pool disp
 * (0x203430+0xD0=0x203500, 0x203454+0x122=0x203576, 0x203460+0x30C=0x20376c,
 * 0x203470+0x68C=0x203afc, 0x203480+0x7A8=0x203c28). cfg byte read unsigned. */
#define GU8(a)  (*(u8  *)(uintptr_t)(a))
#define G32(a)  (*(s32 *)(uintptr_t)(a))

extern void hud_blink_status_task(void *task);
extern void hud_status_two_players_task(void *task);
extern void hud_status_service_2p(void *task);
extern void hud_status_one_player_task(void *task);
extern void hud_status_config_task(void *task);

void hud_install_status_task(void *task) {
    void *entity = TASK_PTR(task, work);
    u8 cfg;

    (*(void (**)(void *))(uintptr_t)0x06000660)((void *)(uintptr_t)0x060987d8);

    if (GU8(0x060987e8) & 1) {
        TASK_PTR(task, handler) = (void *)hud_blink_status_task;
    } else {
        cfg = GU8(0x060987e8);
        if (cfg & 4)
            TASK_PTR(task, handler) = (cfg & 2) ? (void *)hud_status_service_2p : (void *)hud_status_two_players_task;
        else
            TASK_PTR(task, handler) = (cfg & 2) ? (void *)hud_status_config_task : (void *)hud_status_one_player_task;
    }

    if (G32(DH_SCENE_INDEX_ADDR) == DH_SCENE_PLAY_ENTRY || G32(DH_SCENE_INDEX_ADDR) == 6)
        M32(entity, 0x50) = 0x1f;
    else
        M32(entity, 0x50) = 0xf;
}
